This project current has two parts:

  1. A project which wraps a Juce AudioProcessor as a Max/MSP signal processing object
    * This works in MaxMSP 4.6 with a GUI window editor for the plugin and without the GUI in Max 5.
    * Mac / Windows.
  1. A project which wraps a Juce Component as a MaxUI box or places the Juce Component in a window owned by a regular object.
    * This is the same project with two different targets as the code for getting/setting attributes for each of the Component's sliders is the same, for example.
    * Max 4.6 only at present.
    * Mac only at present.