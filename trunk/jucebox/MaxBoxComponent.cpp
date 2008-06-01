#include "../../../juce_code/juce/juce.h"

class EditorComponent : public Component
{
public:
	EditorComponent()
	{
		addAndMakeVisible(slider = new Slider("Slider"));
		slider->setRange(0.0, 1.0, 0.001);
		
		addAndMakeVisible(group = new GroupComponent("Group", "Group"));
		
		group->addAndMakeVisible(dial1 = new Slider("Dial1"));
		dial1->setSliderStyle(Slider::RotaryVerticalDrag);
		dial1->setRange(-50.0, 50.0, 0.1);
		
		group->addAndMakeVisible(dial2 = new Slider("Dial2"));
		dial2->setSliderStyle(Slider::RotaryVerticalDrag);
		dial2->setRange(0.0, 1.0, 0.01);
	}

	~EditorComponent()
	{
		group->deleteAllChildren();
		deleteAllChildren();
	}

	void resized()
	{
		slider->setBounds(20, 20, 200, 24);
		group->setBounds(10, 55, 250, 80);
		dial1->setBounds(10, 10, 100, 60);
		dial2->setBounds(120, 10, 100, 60);
	}

	void paint (Graphics& g)
	{
		g.fillAll (Colour::greyLevel (0.9f));
	}

private:
	Slider* slider;
	GroupComponent *group;
	Slider* dial1;
	Slider* dial2;
};

Component* createMaxBoxComponent()
{
    return new EditorComponent();
}

