#include <FL/fl.h>
#include <FL/Fl_Box.h>

// for using the real time clock
#include <time.h>

#include "TrainWindow.H"
#include "TrainView.H"
#include "CallBacks.H"



//************************************************************************
//
// * Constructor
//========================================================================
TrainWindow::
TrainWindow(const int x, const int y) 
	: Fl_Double_Window(x,y,850,900,"Train and Roller Coaster")
//========================================================================
{
	// make all of the widgets
	begin();	// add to this widget
	{
		int pty=5;			// where the last widgets were drawn

		trainView = new TrainView(5,5,590,890);
		trainView->tw = this;
		trainView->m_pTrack = &m_Track;
		this->resizable(trainView);

		// to make resizing work better, put all the widgets in a group
		widgets = new Fl_Group(600,5,190,790);
		widgets->begin();

		runButton = new Fl_Button(605,pty,60,20,"Run");
		togglify(runButton);
		Fl_Button* fb = new Fl_Button(700,pty,25,20,"@>>");
		fb->callback((Fl_Callback*)forwCB,this);
		Fl_Button* rb = new Fl_Button(670,pty,25,20,"@<<");
		rb->callback((Fl_Callback*)backCB,this);
		arcLength = new Fl_Button(730,pty,65,20,"ArcLength");
		togglify(arcLength,1);
		pty+=25;

		speed = new Fl_Value_Slider(655,pty,140,20,"speed");
		speed->range(0,10);
		speed->value(2);
		speed->align(FL_ALIGN_LEFT);
		speed->type(FL_HORIZONTAL);
		pty += 30;

		// camera buttons - in a radio button group
		Fl_Group* viewGroup = new Fl_Group(600, pty, 195, 20);
		viewGroup->begin();
		arcball = new Fl_Button(605, pty, 60, 20, "Arcball");
		arcball->type(FL_RADIO_BUTTON);		// radio button
		arcball->value(1);			// turned on
		arcball->selection_color((Fl_Color)3); // yellow when pressed
		arcball->callback((Fl_Callback*)damageCB, this);
		fpv = new Fl_Button(670, pty, 60, 20, "FPV");
		fpv->type(FL_RADIO_BUTTON);
		fpv->value(0);
		fpv->selection_color((Fl_Color)3);
		fpv->callback((Fl_Callback*)damageCB, this);
		viewGroup->end();
		pty += 30;

		// camera buttons - in a radio button group
		Fl_Group* camGroup = new Fl_Group(600,pty,195,20);
		camGroup->begin();
		worldCam = new Fl_Button(605, pty, 60, 20, "World");
        worldCam->type(FL_RADIO_BUTTON);		// radio button
        worldCam->value(1);			// turned on
        worldCam->selection_color((Fl_Color)3); // yellow when pressed
		worldCam->callback((Fl_Callback*)damageCB,this);
		trainCam = new Fl_Button(670, pty, 60, 20, "Train");
        trainCam->type(FL_RADIO_BUTTON);
        trainCam->value(0);
        trainCam->selection_color((Fl_Color)3);
		trainCam->callback((Fl_Callback*)damageCB,this);
		topCam = new Fl_Button(735, pty, 60, 20, "Top");
        topCam->type(FL_RADIO_BUTTON);
        topCam->value(0);
        topCam->selection_color((Fl_Color)3);
		topCam->callback((Fl_Callback*)damageCB,this);
		camGroup->end();
		pty += 30;

		// FPV buttons - in a radio button group
		Fl_Group* FPVGroup = new Fl_Group(600, pty, 195, 20);
		FPVGroup->begin();
		FPV = new Fl_Button(605, pty, 60, 20, "FPV");
		FPV->type(FL_RADIO_BUTTON);		// radio button
		FPV->value(1);			// turned on
		FPV->selection_color((Fl_Color)3); // yellow when pressed
		FPV->callback((Fl_Callback*)damageCB, this);
		TPV = new Fl_Button(670, pty, 60, 20, "TPV");
		TPV->type(FL_RADIO_BUTTON);
		TPV->value(0);
		TPV->selection_color((Fl_Color)3);
		TPV->callback((Fl_Callback*)damageCB, this);
		FPVGroup->end();
		pty += 30;

		pickObjects = new Fl_Button(605, pty, 100, 20, "Pick");
		togglify(pickObjects, 0);
		deleteObjects = new Fl_Button(700, pty, 100, 20, "Delete");
		deleteObjects->callback((Fl_Callback*)delObject, this);
		pty += 30;

		AddTree = new Fl_Button(605, pty, 100, 20, "Add Tree");
		AddTree->callback((Fl_Callback*)addTree, this);
		pty += 30;

		// browser to select spline types
		splineBrowser = new Fl_Browser(605,pty,120,75,"Spline Type");
		splineBrowser->type(2);		// select
		splineBrowser->callback((Fl_Callback*)damageCB,this);
		splineBrowser->add("Linear");
		splineBrowser->add("Cardinal Cubic");
		splineBrowser->add("Cubic B-Spline");
		splineBrowser->select(2);
		pty += 110;

		// add and delete points
		Fl_Button* ap = new Fl_Button(605,pty,80,20,"Add Point");
		ap->callback((Fl_Callback*)addPointCB,this);
		Fl_Button* dp = new Fl_Button(690,pty,80,20,"Delete Point");
		dp->callback((Fl_Callback*)deletePointCB,this);
		pty += 25;

		// reset the points
		resetButton = new Fl_Button(735,pty,60,20,"Reset");
		resetButton->callback((Fl_Callback*)resetCB,this);
		Fl_Button* loadb = new Fl_Button(605,pty,60,20,"Load");
		loadb->callback((Fl_Callback*) loadCB, this);
		Fl_Button* saveb = new Fl_Button(670,pty,60,20,"Save");
		saveb->callback((Fl_Callback*) saveCB, this);
		pty += 25;

		// roll the points
		Fl_Button* rx = new Fl_Button(605,pty,30,20,"R+X");
		rx->callback((Fl_Callback*)rpxCB,this);
		Fl_Button* rxp = new Fl_Button(635,pty,30,20,"R-X");
		rxp->callback((Fl_Callback*)rmxCB,this);
		Fl_Button* rz = new Fl_Button(670,pty,30,20,"R+Z");
		rz->callback((Fl_Callback*)rpzCB,this);
		Fl_Button* rzp = new Fl_Button(700,pty,30,20,"R-Z");
		rzp->callback((Fl_Callback*)rmzCB,this);
		pty+=30;

		waveTypeBrowser = new Fl_Browser(605, pty, 120, 75, "wave Type");
		waveTypeBrowser->type(1);		// select
		waveTypeBrowser->callback((Fl_Callback*)damageCB, this);
		waveTypeBrowser->add("Sine wave");
		waveTypeBrowser->add("Height map");
		waveTypeBrowser->select(1);
		pty += 100;

		waterAmplitude = new Fl_Value_Slider(675, pty, 120, 20, "Amplitude");
		waterAmplitude->range(0, 10);
		waterAmplitude->value(1);
		waterAmplitude->align(FL_ALIGN_LEFT);
		waterAmplitude->type(FL_HORIZONTAL);
		pty += 25;

		waterWaveLength = new Fl_Value_Slider(675, pty, 120, 20, "Wave length");
		waterWaveLength->range(0, 10);
		waterWaveLength->value(5);
		waterWaveLength->align(FL_ALIGN_LEFT);
		waterWaveLength->type(FL_HORIZONTAL);
		pty += 25;

		waterSpeed = new Fl_Value_Slider(675, pty, 120, 20, "Wave Speed");
		waterSpeed->range(0, 10);
		waterSpeed->value(5);
		waterSpeed->align(FL_ALIGN_LEFT);
		waterSpeed->type(FL_HORIZONTAL);
		pty += 30;

		envLight = new Fl_Button(605, pty, 100, 20, "Lighting");
		togglify(envLight, 1);
		pty += 30;

		physics = new Fl_Button(605, pty, 100, 20, "Physics");
		togglify(physics, 1);
		pty += 30;

		//add a car
		add_car = new Fl_Button(605, pty, 80, 20, "Add car");
		add_car->callback((Fl_Callback*)addCar, this);
		//delete a car
		delete_car = new Fl_Button(700, pty, 80, 20, "Delete car");
		delete_car->callback((Fl_Callback*)delCar, this);
		pty += 30;




		//pixelation = new Fl_Button(605, pty, 80, 20, "Pixelation");
		//togglify(pixelation);

		//pty += 30;

		//offset = new Fl_Button(605, pty, 80, 20, "Offset");
		//togglify(offset);

		//pty += 30;

		//grayscale = new Fl_Button(605, pty, 80, 20, "Grayscale");
		//togglify(grayscale);

		//pty += 30;

		// TODO: add widgets for all of your fancier features here


		// we need to make a little phantom widget to have things resize correctly
		Fl_Box* resizebox = new Fl_Box(600,595,200,5);
		widgets->resizable(resizebox);

		widgets->end();
	}
	end();	// done adding to this widget

	// set up callback on idle
	Fl::add_idle((void (*)(void*))runButtonCB,this);
}

//************************************************************************
//
// * handy utility to make a button into a toggle
//========================================================================
void TrainWindow::
togglify(Fl_Button* b, int val)
//========================================================================
{
	b->type(FL_TOGGLE_BUTTON);		// toggle
	b->value(val);		// turned off
	b->selection_color((Fl_Color)3); // yellow when pressed	
	b->callback((Fl_Callback*)damageCB,this);
}

//************************************************************************
//
// *
//========================================================================
void TrainWindow::
damageMe()
//========================================================================
{
	if (trainView->selectedCube >= ((int)m_Track.points.size()))
		trainView->selectedCube = 0;
	trainView->damage(1);
}

//************************************************************************
//
// * This will get called (approximately) 30 times per second
//   if the run button is pressed
//========================================================================
void TrainWindow::
advanceTrain(float dir)
//========================================================================
{
	//#####################################################################
	// TODO: make this work for your train
	//#####################################################################
	float speed_val0 = dir * this->speed->value() / 100.0;
	float speed_val1 = dir * this->speed->value();

	if (physics->value() == 1) {
		size_t i;
		i = trainView->C_length_index();
		speed_val1 = speed_val1 * trainView->speeds[i];
	}
#ifdef DEBUG
	//cout << "s: " << speed_val1 << endl;
	//speed_val = 0.001;
#endif // DEBUG

	if (arcLength->value() == 1) {
		trainView->m_pTrack->C_length += speed_val1;
		if (trainView->m_pTrack->C_length >= trainView->accumulate_length.back()) {
			trainView->m_pTrack->C_length = 0;
			trainView->m_pTrack->trainU = 0;
	}
		trainView->match_length();

}
	else {
		trainView->m_pTrack->trainU += speed_val0;
		trainView->match_trainU();
	}

	if (trainView->m_pTrack->trainU >= trainView->m_pTrack->points.size() - 0.01) {
		trainView->m_pTrack->trainU = 0;
	}
}