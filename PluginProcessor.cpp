#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

using APVTS = juce::AudioProcessorValueTreeState;

namespace
{
    // Start-Pattern. Note = Halbtöne über ROOT; 12 entspricht der mittleren Oktave der Write-Tastatur.
    const int  defNote[16]  = { 12, 12, 24, 12, 15, 12, 22, 12, 12, 24, 12, 19, 17, 12, 15, 27 };
    const bool defGate[16]  = { 1, 1, 1,  0, 1, 1, 1,  0, 1, 1,  0, 1, 1, 1, 1, 1 };
    const bool defAcc[16]   = { 0, 0, 1,  0, 0, 0, 1,  0, 0, 1,  0, 0, 1, 0, 0, 1 };
    const bool defSlide[16] = { 0, 0, 0,  0, 0, 1, 0,  0, 0, 0,  0, 1, 0, 0, 1, 0 };
}

Acid303Processor::Acid303Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto get = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };

    pWave = get ("wave");   pFilter = get ("filter"); pTune = get ("tune");   pCutoff = get ("cutoff");
    pReso = get ("reso");   pEnvMod = get ("envmod"); pDecay = get ("decay");
    pAccent = get ("accent"); pSlide = get ("slide"); pVolume = get ("volume");
    pRun = get ("run");     pSync = get ("sync");   pBpm = get ("bpm");
    pRoot = get ("root");   pLength = get ("length"); pGateLen = get ("gatelen");

    for (int i = 0; i < kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        pNote[i]      = get ("note" + n);
        pGate[i]      = get ("gate" + n);
        pTie[i]       = get ("tie" + n);
        pAcc[i]       = get ("acc" + n);
        pStepSlide[i] = get ("slide" + n);
    }
}

APVTS::ParameterLayout Acid303Processor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using I = juce::AudioParameterInt;
    using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>;
    APVTS::ParameterLayout l;

    // Klang
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "wave", 1 }, "Waveform",
                                                         juce::StringArray { "Saw", "Square" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "filter", 1 }, "Filter Slope",
                                                         juce::StringArray { "12 dB", "24 dB" }, 0));   // 12 dB = Hauptfilter
    l.add (std::make_unique<F> (juce::ParameterID { "tune", 1 },   "Tune (cents)", R (-100.f, 100.f, 0.1f), 0.f));
    l.add (std::make_unique<F> (juce::ParameterID { "cutoff", 1 }, "Cutoff",       R (50.f, 8000.f, 0.f, 0.3f), 400.f));
    l.add (std::make_unique<F> (juce::ParameterID { "reso", 1 },   "Resonance",    R (0.f, 1.f), 0.7f));
    l.add (std::make_unique<F> (juce::ParameterID { "envmod", 1 }, "Env Mod",      R (0.f, 1.f), 0.6f));
    l.add (std::make_unique<F> (juce::ParameterID { "decay", 1 },  "Decay (s)",    R (0.08f, 2.0f, 0.f, 0.5f), 0.4f));
    l.add (std::make_unique<F> (juce::ParameterID { "accent", 1 }, "Accent",       R (0.f, 1.f), 0.7f));
    l.add (std::make_unique<F> (juce::ParameterID { "slide", 1 },  "Slide (ms)",   R (10.f, 300.f, 0.f, 0.5f), 60.f));
    l.add (std::make_unique<F> (juce::ParameterID { "volume", 1 }, "Volume",       R (0.f, 1.f), 0.7f));

    // Sequencer global
    l.add (std::make_unique<B> (juce::ParameterID { "run", 1 },  "Run",       false));
    l.add (std::make_unique<B> (juce::ParameterID { "sync", 1 }, "Host Sync", false));
    l.add (std::make_unique<F> (juce::ParameterID { "bpm", 1 },  "Tempo",     R (50.f, 220.f, 0.1f), 130.f));
    l.add (std::make_unique<I> (juce::ParameterID { "root", 1 }, "Root Note", 24, 60, 24));
    l.add (std::make_unique<I> (juce::ParameterID { "length", 1 }, "Length",  1, 16, 16));
    l.add (std::make_unique<F> (juce::ParameterID { "gatelen", 1 }, "Gate Length", R (0.1f, 0.95f), 0.55f));

    // Pro Step
    for (int i = 0; i < kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        l.add (std::make_unique<I> (juce::ParameterID { "note" + n, 1 },  "Step " + n + " Note",   0, 36, defNote[i]));
        l.add (std::make_unique<B> (juce::ParameterID { "gate" + n, 1 },  "Step " + n + " Gate",   defGate[i]));
        l.add (std::make_unique<B> (juce::ParameterID { "tie" + n, 1 },   "Step " + n + " Tie",    false));
        l.add (std::make_unique<B> (juce::ParameterID { "acc" + n, 1 },   "Step " + n + " Accent", defAcc[i]));
        l.add (std::make_unique<B> (juce::ParameterID { "slide" + n, 1 }, "Step " + n + " Slide",  defSlide[i]));
    }
    return l;
}

juce::AudioProcessorEditor* Acid303Processor::createEditor() { return new Acid303Editor (*this); }

void Acid303Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (samplesPerBlock, 64), 1 };
    filter.prepare (spec);
    filter.setMode (juce::dsp::LadderFilter<float>::Mode::LPF12);
    filter.setDrive (1.5f);
    filter.reset();
    currentFilterMode = -1;

    heldNotes.clear();
    gate = false;
    ampEnv = filterEnv = accentEnv = 0.0f;
    wasRunning = false;
    auditionNote = -1;
    stopSequencer();
}

float Acid303Processor::polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt;             return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// ---------------------------------------------------------------- Synth

void Acid303Processor::noteOn (int note, float velocity)
{
    const bool legato = ! heldNotes.empty();
    heldNotes.push_back (note);

    targetFreq = 440.0f * std::pow (2.0f, ((float) note - 69.0f) / 12.0f + pTune->load() / 1200.0f);
    accent = velocity >= 0.78f;

    if (legato)
    {
        sliding = true;       // überlappende Noten = Slide, kein neuer Hüllkurven-Trigger
    }
    else
    {
        sliding = false;
        freq = targetFreq;
        filterEnv = 1.0f;
        gate = true;
    }
    if (accent) accentEnv = 1.0f;
}

void Acid303Processor::noteOff (int note)
{
    // Nur eine Instanz entfernen (Slide auf gleiche Tonhöhe erzeugt Duplikate)
    auto it = std::find (heldNotes.begin(), heldNotes.end(), note);
    if (it != heldNotes.end())
        heldNotes.erase (it);

    if (heldNotes.empty())
    {
        gate = false;
    }
    else
    {
        targetFreq = 440.0f * std::pow (2.0f, ((float) heldNotes.back() - 69.0f) / 12.0f + pTune->load() / 1200.0f);
        sliding = true;
    }
}

void Acid303Processor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())       noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff()) noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) { heldNotes.clear(); gate = false; seqNote = -1; }
}

void Acid303Processor::renderChunk (float* out, int numSamples)
{
    if (numSamples <= 0) return;

    const bool  square  = (int) pWave->load() == 1;
    const float cutoff  = pCutoff->load();
    const float reso    = pReso->load();
    const float envMod  = pEnvMod->load();
    const float decay   = pDecay->load();
    const float accAmt  = pAccent->load();
    const float slideMs = pSlide->load();
    const float volume  = pVolume->load();

    // Hauptfilter: 12 dB (Standard) oder 24 dB
    const int filterMode = (int) pFilter->load();
    if (filterMode != currentFilterMode)
    {
        currentFilterMode = filterMode;
        filter.setMode (filterMode == 0 ? juce::dsp::LadderFilter<float>::Mode::LPF12
                                        : juce::dsp::LadderFilter<float>::Mode::LPF24);
    }

    const float fsr = (float) sr;
    const float slideCoef = 1.0f - std::exp (-1.0f / (slideMs * 0.001f * fsr));
    const float ampAtk    = 1.0f - std::exp (-1.0f / (0.003f * fsr));
    const float ampRel    = 1.0f - std::exp (-1.0f / (0.012f * fsr));
    const float accDecay  = std::exp (-1.0f / (0.2f * fsr));

    constexpr int sub = 16;
    float tmp[sub];

    for (int start = 0; start < numSamples; start += sub)
    {
        const int n = juce::jmin (sub, numSamples - start);

        const float dec = decay * (accent ? (1.0f - 0.6f * accAmt) : 1.0f);
        const float envCoef = std::exp (-1.0f / (juce::jmax (dec, 0.02f) * fsr));
        const float mod = envMod * (1.0f + (accent ? accAmt * accentEnv : 0.0f));

        for (int i = 0; i < n; ++i)
        {
            if (sliding) freq += (targetFreq - freq) * slideCoef;

            const float dt = freq / fsr;
            phase += dt;
            if (phase >= 1.0) phase -= 1.0;
            const float p = (float) phase;

            float s;
            if (square)
            {
                s = p < 0.5f ? 1.0f : -1.0f;
                s += polyBlep (p, dt);
                s -= polyBlep (std::fmod (p + 0.5f, 1.0f), dt);
            }
            else
            {
                s = 2.0f * p - 1.0f - polyBlep (p, dt);
            }

            ampEnv += ((gate ? 1.0f : 0.0f) - ampEnv) * (gate ? ampAtk : ampRel);
            filterEnv *= envCoef;
            accentEnv *= accDecay;

            tmp[i] = s * ampEnv * (accent ? 1.0f + 0.5f * accAmt * accentEnv : 1.0f);
        }

        const float fc = juce::jlimit (30.0f, 18000.0f, cutoff * std::pow (2.0f, 5.0f * mod * filterEnv));
        filter.setCutoffFrequencyHz (fc);
        filter.setResonance (juce::jlimit (0.0f, 0.95f, reso));

        float* ch[1] = { tmp };
        juce::dsp::AudioBlock<float> block (ch, 1, (size_t) n);
        filter.process (juce::dsp::ProcessContextReplacing<float> (block));

        for (int i = 0; i < n; ++i)
            out[start + i] = tmp[i] * volume * 0.5f;
    }
}

// ------------------------------------------------------------ Sequencer

void Acid303Processor::stopSequencer()
{
    if (seqNote >= 0) noteOff (seqNote);
    seqNote = -1;
    gateCountdown = -1;
    lastTriggered = -1;
    currentStep.store (-1);
}

void Acid303Processor::triggerStep (int stepInt, int length, double inc)
{
    lastTriggered = stepInt;
    const int idx = ((stepInt % length) + length) % length;
    const int nextIdx = (idx + 1) % length;
    currentStep.store (idx);

    const bool on  = pGate[idx]->load() > 0.5f;
    const bool tie = pTie[idx]->load()  > 0.5f;
    const bool acc = pAcc[idx]->load()  > 0.5f;
    const bool sld = pStepSlide[idx]->load() > 0.5f;
    const bool nextTie = pGate[nextIdx]->load() > 0.5f && pTie[nextIdx]->load() > 0.5f;
    const int note = juce::jlimit (0, 127, (int) std::lround (pRoot->load()) + (int) std::lround (pNote[idx]->load()));
    const int gateSamples = juce::jmax (1, (int) (pGateLen->load() / inc));

    // Tie: Die vorherige Note klingt über diesen Step einfach weiter (kein neuer Anschlag)
    if (on && tie && seqNote >= 0)
    {
        gateCountdown = (sld || nextTie) ? -1 : gateSamples;
        return;
    }

    // Erst die neue Note an, dann die vorherige (gehaltene) Note aus:
    // so entsteht bei einem Slide-Step ein Legato-Übergang mit Pitch-Glide.
    const bool play = on && ! tie;
    if (play) noteOn (note, acc ? 1.0f : 0.6f);
    if (seqNote >= 0) noteOff (seqNote);
    seqNote = play ? note : -1;

    // Slide oder nachfolgender Tie: Gate bleibt bis zum nächsten Step offen
    gateCountdown = (play && ! (sld || nextTie)) ? gateSamples : -1;
}

void Acid303Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int n = buffer.getNumSamples();
    float* out = buffer.getWritePointer (0);

    // Vorhören einer Note aus dem Editor (Write-Modus): kurze Note per MIDI-Event
    const int aReq = auditionRequest.exchange (-1);
    if (aReq >= 0)
    {
        if (auditionNote >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, auditionNote), 0);
        auditionNote = aReq;
        midi.addEvent (juce::MidiMessage::noteOn (1, auditionNote, (juce::uint8) 90), 0);
        auditionSamplesLeft = (int) (sr * 0.2);
    }
    if (auditionNote >= 0)
    {
        if (auditionSamplesLeft < n)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, auditionNote), juce::jmax (0, auditionSamplesLeft));
            auditionNote = -1;
        }
        else
        {
            auditionSamplesLeft -= n;
        }
    }

    // Transport
    const bool runParam  = pRun->load()  > 0.5f;
    const bool syncParam = pSync->load() > 0.5f;
    double bpm = pBpm->load();
    bool hostPlaying = true, haveHostPpq = false;
    double hostPpq = 0.0;

    if (syncParam)
    {
        hostPlaying = false;
        if (auto* ph = getPlayHead())
        {
            if (auto pos = ph->getPosition())
            {
                hostPlaying = pos->getIsPlaying();
                if (auto b = pos->getBpm())         bpm = *b;
                if (auto p = pos->getPpqPosition()) { hostPpq = *p; haveHostPpq = true; }
            }
        }
    }

    const bool running = runParam && (! syncParam || hostPlaying);
    const double inc = bpm * 4.0 / (60.0 * sr);          // 16tel-Schritte pro Sample
    const int length = juce::jlimit (1, kSteps, (int) std::lround (pLength->load()));

    if (running && ! wasRunning)
    {
        stepPos = (syncParam && haveHostPpq) ? hostPpq * 4.0 : 0.0;
        lastTriggered = -1;
    }
    else if (! running && wasRunning)
    {
        stopSequencer();
    }
    else if (running && syncParam && haveHostPpq)
    {
        const double hs = hostPpq * 4.0;
        if (std::abs (hs - stepPos) > 0.05) stepPos = hs;  // Sprung / Loop im Host
    }
    wasRunning = running;

    // Sample-genaue Verarbeitung: MIDI-Events, Step-Grenzen und Gate-Ende
    auto midiIt = midi.begin();
    const auto midiEnd = midi.end();
    int pos = 0;

    while (pos < n)
    {
        int next = n;

        if (midiIt != midiEnd)
            next = juce::jmin (next, juce::jmax (pos, (*midiIt).samplePosition));

        if (running)
        {
            if ((int) std::floor (stepPos) != lastTriggered)
                next = pos;                                    // Step sofort auslösen
            else
            {
                const double toBoundary = (std::floor (stepPos) + 1.0 - stepPos) / inc;
                next = juce::jmin (next, pos + (int) std::ceil (toBoundary));
            }
        }

        if (gateCountdown >= 0)
            next = juce::jmin (next, pos + gateCountdown);

        next = juce::jlimit (pos, n, next);

        renderChunk (out + pos, next - pos);

        if (running) stepPos += inc * (double) (next - pos);
        if (gateCountdown >= 0) gateCountdown -= (next - pos);
        pos = next;

        while (midiIt != midiEnd && (*midiIt).samplePosition <= pos)
        {
            handleMidi ((*midiIt).getMessage());
            ++midiIt;
        }

        if (gateCountdown == 0)
        {
            if (seqNote >= 0) noteOff (seqNote);
            seqNote = -1;
            gateCountdown = -1;
        }

        if (running && (int) std::floor (stepPos) != lastTriggered)
            triggerStep ((int) std::floor (stepPos), length, inc);
    }

    for (int c = 1; c < buffer.getNumChannels(); ++c)
        buffer.copyFrom (c, 0, buffer, 0, 0, n);
}

void Acid303Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void Acid303Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Acid303Processor(); }
