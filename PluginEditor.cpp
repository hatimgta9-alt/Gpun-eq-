#include "PluginEditor.h"

static juce::Colour bg (0xff10131a), panel (0xff191e28), accent (0xff39d5b5),
                     textCol (0xffe8edf5), muted (0xff8792a5);

GPUNEQAudioProcessorEditor::GPUNEQAudioProcessorEditor(GPUNEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(900, 610);
    titleLabel.setText("INFINITY EQ", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, textCol);
    addAndMakeVisible(titleLabel);

    for (auto* b : { &sizeButton, &addButton, &removeButton })
    {
        b->setColour(juce::TextButton::buttonColourId, panel);
        b->setColour(juce::TextButton::textColourOffId, textCol);
        addAndMakeVisible(*b);
    }
    sizeButton.onClick = [this] {
        compact = !compact;
        sizeButton.setButtonText(compact ? "EXPAND" : "COMPACT");
        setSize(compact ? 520 : 900, compact ? 390 : 610);
        resized();
    };
    addButton.onClick = [this] {
        audioProcessor.addBand();
        selectedBand = audioProcessor.getBandCount()-1;
        rebuildBandControls();
        repaint();
    };
    removeButton.onClick = [this] {
        audioProcessor.removeBand(selectedBand);
        selectedBand = juce::jmax(0, selectedBand-1);
        rebuildBandControls();
        repaint();
    };

    bandSelector.setColour(juce::ComboBox::backgroundColourId, panel);
    typeSelector.setColour(juce::ComboBox::backgroundColourId, panel);
    typeSelector.addItem("Bell", 1);
    typeSelector.addItem("Low Shelf", 2);
    typeSelector.addItem("High Shelf", 3);
    typeSelector.addItem("High Pass", 4);
    typeSelector.addItem("Low Pass", 5);
    addAndMakeVisible(bandSelector);
    addAndMakeVisible(typeSelector);
    bandSelector.onChange = [this] {
        selectedBand = bandSelector.getSelectedItemIndex();
        updateSelectedBand();
        repaint();
    };
    typeSelector.onChange = [this] { applyControls(); repaint(); };

    auto setupSlider = [this](juce::Slider& s, const juce::String& suffix, double min, double max, double step) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 22);
        s.setRange(min, max, step);
        s.setTextValueSuffix(suffix);
        s.setColour(juce::Slider::rotarySliderFillColourId, accent);
        s.setColour(juce::Slider::textBoxTextColourId, textCol);
        s.setColour(juce::Slider::textBoxBackgroundColourId, panel);
        addAndMakeVisible(s);
        s.onValueChange = [this] { applyControls(); repaint(); };
    };
    setupSlider(frequency, " Hz", 20, 20000, 1);
    setupSlider(gain, " dB", -24, 24, 0.1);
    setupSlider(q, " Q", 0.1, 18, 0.01);

    for (auto pair : { std::pair<juce::Label*, juce::String>{&freqLabel,"FREQUENCY"},
                        {&gainLabel,"GAIN"}, {&qLabel,"Q / WIDTH"} })
    {
        pair->first->setText(pair->second, juce::dontSendNotification);
        pair->first->setJustificationType(juce::Justification::centred);
        pair->first->setColour(juce::Label::textColourId, muted);
        addAndMakeVisible(*pair->first);
    }
    rebuildBandControls();
    startTimerHz(12);
}

void GPUNEQAudioProcessorEditor::rebuildBandControls()
{
    bandSelector.clear();
    const int count = audioProcessor.getBandCount();
    for (int i=0; i<count; ++i) bandSelector.addItem("Band " + juce::String(i+1), i+1);
    selectedBand = juce::jlimit(0, count-1, selectedBand);
    bandSelector.setSelectedItemIndex(selectedBand, juce::dontSendNotification);
    removeButton.setEnabled(count > 1);
    updateSelectedBand();
}
void GPUNEQAudioProcessorEditor::updateSelectedBand()
{
    auto b = audioProcessor.getBand(selectedBand);
    frequency.setValue(b.frequency, juce::dontSendNotification);
    gain.setValue(b.gain, juce::dontSendNotification);
    q.setValue(b.q, juce::dontSendNotification);
    typeSelector.setSelectedId(b.type+1, juce::dontSendNotification);
}
void GPUNEQAudioProcessorEditor::applyControls()
{
    auto b = audioProcessor.getBand(selectedBand);
    b.frequency = (float)frequency.getValue();
    b.gain = (float)gain.getValue();
    b.q = (float)q.getValue();
    b.type = typeSelector.getSelectedId()-1;
    audioProcessor.updateBand(selectedBand, b);
}
void GPUNEQAudioProcessorEditor::drawResponse(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(panel); g.fillRoundedRectangle(r, 12.0f);
    for (int i=0; i<=4; ++i)
    {
        float y = r.getY() + r.getHeight() * i / 4.0f;
        g.setColour(juce::Colour(0xff2a3240)); g.drawHorizontalLine((int)y, r.getX()+8, r.getRight()-8);
    }
    for (int i=0; i<=6; ++i)
    {
        float x = r.getX() + r.getWidth() * i / 6.0f;
        g.setColour(juce::Colour(0xff2a3240)); g.drawVerticalLine((int)x, r.getY()+8, r.getBottom()-8);
    }
    g.setColour(muted);
    g.setFont(10.0f);
    const juce::String labels[] = {"20","50","100","500","1k","5k","20k"};
    for (int i=0; i<7; ++i)
        g.drawText(labels[i], (int)(r.getX()+r.getWidth()*i/6.0f-14), (int)r.getBottom()-18, 32, 14, juce::Justification::centred);

    // Composite magnitude response, calculated from the same biquad coefficients.
    juce::Path path;
    for (int px=0; px<=(int)r.getWidth(); ++px)
    {
        double norm = (double)px / juce::jmax(1.0f, r.getWidth());
        double hz = 20.0 * std::pow(1000.0, norm); // 20 Hz .. 20 kHz
        double mag = 1.0;
        for (int i=0; i<audioProcessor.getBandCount(); ++i)
        {
            auto b = audioProcessor.getBand(i);
            double f=juce::jlimit(10.0, 20000.0, (double)b.frequency);
            double qv=juce::jlimit(0.1,18.0,(double)b.q);
            double A=std::pow(10.0,(double)b.gain/40.0);
            double w=2.0*juce::MathConstants<double>::pi*f/44100.0;
            double c=std::cos(w), s=std::sin(w), alpha=s/(2*qv);
            double b0=1,b1=0,b2=0,a0=1,a1=0,a2=0;
            if (b.type==3 || b.type==4) {
                if (b.type==3) { b0=(1+c)/2; b1=-(1+c); b2=b0; }
                else { b0=(1-c)/2; b1=1-c; b2=b0; }
                a0=1+alpha; a1=-2*c; a2=1-alpha;
            } else if (b.type==1 || b.type==2) {
                double beta=2*std::sqrt(A)*alpha;
                if (b.type==1) {
                    b0=A*((A+1)-(A-1)*c+beta); b1=2*A*((A-1)-(A+1)*c); b2=A*((A+1)-(A-1)*c-beta);
                    a0=(A+1)+(A-1)*c+beta; a1=-2*((A-1)+(A+1)*c); a2=(A+1)+(A-1)*c-beta;
                } else {
                    b0=A*((A+1)+(A-1)*c+beta); b1=-2*A*((A-1)+(A+1)*c); b2=A*((A+1)+(A-1)*c-beta);
                    a0=(A+1)-(A-1)*c+beta; a1=2*((A-1)-(A+1)*c); a2=(A+1)-(A-1)*c-beta;
                }
            } else {
                b0=1+alpha*A; b1=-2*c; b2=1-alpha*A;
                a0=1+alpha/A; a1=-2*c; a2=1-alpha/A;
            }
            double omega=2.0*juce::MathConstants<double>::pi*hz/44100.0;
            auto eval = [omega](double x0,double x1,double x2) {
                auto re=x0+x1*std::cos(omega)+x2*std::cos(2*omega);
                auto im=-x1*std::sin(omega)-x2*std::sin(2*omega);
                return std::sqrt(re*re+im*im);
            };
            mag *= eval(b0,b1,b2)/juce::jmax(1.0e-12,eval(a0,a1,a2));
        }
        double db=juce::Decibels::gainToDecibels(mag, -36.0);
        float y=juce::jlimit(r.getY()+5, r.getBottom()-20,
            r.getCentreY()-(float)(db/36.0)*r.getHeight()*0.45f);
        if (px==0) path.startNewSubPath(r.getX(),y); else path.lineTo(r.getX()+px,y);
    }
    g.setColour(accent); g.strokePath(path, juce::PathStrokeType(2.2f));
    g.setColour(textCol); g.drawText("0 dB", (int)r.getX()+8, (int)r.getCentreY()-14, 36, 14, juce::Justification::left);
}

void GPUNEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto area=getLocalBounds().toFloat().reduced(18.0f);
    g.setColour(textCol);
    g.setFont(15.0f);
    g.drawText("Dynamic parametric equalizer  •  20 Hz – 20 kHz", 20, 48, getWidth()-40, 22, juce::Justification::left);
    auto graph = compact ? area.removeFromTop(155.0f).withTrimmedTop(58.0f) : area.removeFromTop(300.0f).withTrimmedTop(70.0f);
    drawResponse(g, graph);
    g.setColour(muted);
    g.setFont(11.0f);
    g.drawText(juce::String(audioProcessor.getBandCount()) + " active bands  •  Add as many as your session needs",
               20, getHeight()-20, getWidth()-40, 14, juce::Justification::centred);
}
void GPUNEQAudioProcessorEditor::resized()
{
    auto w=getWidth();
    titleLabel.setBounds(20, 12, 240, 32);
    sizeButton.setBounds(w-120, 14, 100, 30);
    addButton.setBounds(20, compact ? 220 : 390, 120, 30);
    removeButton.setBounds(148, compact ? 220 : 390, 90, 30);
    bandSelector.setBounds(250, compact ? 220 : 390, 110, 30);
    typeSelector.setBounds(370, compact ? 220 : 390, 125, 30);
    int y=compact ? 258 : 432;
    auto place = [y](juce::Label& l, juce::Slider& s, int x) {
        l.setBounds(x, y, 130, 18); s.setBounds(x, y+18, 130, 95);
    };
    place(freqLabel, frequency, 20);
    place(gainLabel, gain, 180);
    place(qLabel, q, 340);
}
void GPUNEQAudioProcessorEditor::timerCallback()
{
    // Keep band menu in sync if a host restores a preset.
    if (bandSelector.getNumItems() != audioProcessor.getBandCount())
        rebuildBandControls();
}
