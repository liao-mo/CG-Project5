/************************************************************************
     File:        TrainWindow.H

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						this class defines the window in which the project 
						runs - its the outer windows that contain all of 
						the widgets, including the "TrainView" which has the 
						actual OpenGL window in which the train is drawn

						You might want to modify this class to add new widgets
						for controlling	your train

						This takes care of lots of things - including installing 
						itself into the FlTk "idle" loop so that we get periodic 
						updates (if we're running the train).


     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/
#pragma once

#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Double_Window.h>
#include <Fl/Fl_Button.h>
#include <Fl/Fl_Group.H>
#include <Fl/Fl_Value_Slider.H>
#include <Fl/Fl_Browser.H>
#pragma warning(pop)

// we need to know what is in the world to show
#include "Track.H"

// other things we just deal with as pointers, to avoid circular references
class TrainView;

// if we're also making the sample solution, then we need to know 
// about the stuff we don't tell students
#ifdef EXAMPLE_SOLUTION
#include "TrainExample/TrainExample.H"
#endif

class TrainWindow : public Fl_Double_Window {
	public:
		TrainWindow(const int x=50, const int y=50);

	public:
		// call this method when things change
		void damageMe();

		// this moves the train forward on the track - its up to you to do this
		// correctly. it gets called from the idle callback loop
		// it should handle forward and backwards
		void advanceTrain(float dir = 1);

		// simple helper function to set up a button
		void togglify(Fl_Button*, int state=0);

	public:
		// keep track of the stuff in the world
		CTrack				m_Track;

		// the widgets that make up the Window
		TrainView*			trainView;

		Fl_Group*			widgets;	// all widgets, grouped for resizing ease

		// utility buttons
		Fl_Button*			resetButton;

		//arcball or fpv
		Fl_Button* arcball;
		Fl_Button* fpv;

		// which viewpoint are we drawing from
		Fl_Button*			worldCam;
		Fl_Button*			trainCam;
		Fl_Button*			topCam;

		// which type of view to follow the train
		Fl_Button* FPV;
		Fl_Button* TPV;

		//want to pick objects?
		Fl_Button* pickObjects;

		//Delete picked object
		Fl_Button* deleteObjects;

		//Add a new Tree
		Fl_Value_Slider* treeSize;
		Fl_Button* AddTree;

		// the type of the spline (use its value to determine)
		Fl_Browser*			splineBrowser;

		// are we animating the train?
		Fl_Button*			runButton;
		// if we're animating it, how fast should it go?
		Fl_Value_Slider*	speed;
		Fl_Button*			arcLength;		// do we use arc length for speed?

		//water adjustment
		Fl_Value_Slider* waterAmplitude;
		Fl_Value_Slider* waterWaveLength;
		Fl_Value_Slider* waterSpeed;

		//wave type browser
		Fl_Browser* waveTypeBrowser;

		////do pixelation
		//Fl_Button* pixelation;

		////do offset
		//Fl_Button* offset;

		////do grayscale
		//Fl_Button* grayscale;

		Fl_Button* physics; // do we want physics?


		// add a car
		Fl_Button* add_car;
		// delete a car
		Fl_Button* delete_car;

		//start kill titan game
		Fl_Button* start_game;
};
