#include "../../../juce_code/juce/juce.h"

class EditorComponent : public Component
{
public:
	EditorComponent()
	{
		addAndMakeVisible(slider = new Slider("Slider"));
		slider->setRange(0.0, 1.0, 0.001);
		addAndMakeVisible(dial = new Slider("Dial"));
		dial->setSliderStyle(Slider::RotaryVerticalDrag);
		dial->setRange(-50.0, 50.0, 0.1);
	}

	~EditorComponent()
	{
		deleteAllChildren();
	}

	void resized()
	{
		slider->setBounds(20, 20, 200, 24);
		dial->setBounds(20, 54, 200, 80);
	}

	void paint (Graphics& g)
	{
		g.fillAll (Colour::greyLevel (0.9f));
	}

private:
	Slider* slider;
	Slider* dial;
};

Component* createMaxBoxComponent()
{
    return new EditorComponent();
}

