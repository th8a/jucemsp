/*
 *  MSPEditorComponent.h
 *  juce_mspobject
 *
 *  Created by Martin Robinson on 26/05/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _MSPEDITORCOMPONENT_H_
#define _MSPEDITORCOMPONENT_H_

#include "MSPAudioProcessor.h"

class MSPEditorComponent : public AudioProcessorEditor
{
public:
	MSPEditorComponent(MSPAudioProcessor* const ownerFilter)
		:	AudioProcessorEditor(ownerFilter)
	{
		addAndMakeVisible(slider = new Slider("Slider"));
		setSize(200,24);
	}

	~MSPEditorComponent()
	{
		deleteAllChildren();
	}

	void resized()
	{
		slider->setBounds(20,20,200,24);
	}

	void paint (Graphics& g)
	{
		g.fillAll (Colour::greyLevel (0.9f));
	}
	

private:
	Slider* slider;
	
	MSPAudioProcessor* getFilter() const throw()       { return (MSPAudioProcessor*)getAudioProcessor(); }
};

#endif // _MSPEDITORCOMPONENT_H_
