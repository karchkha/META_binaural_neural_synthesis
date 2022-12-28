/**
	@file
	binaural_ns
	
	binaural_ns is a MAX/MSP extension, the object that can load and run pytorvh neural network through running the Python server. 
	The the python server will be acquisitively fast and will use the GPU if available. 
	For the object to function, you need Python and Tensorflow installed on your machine. Ecxact list of nessesary librieries will be provided
	with the object in README file. binaural_ns takes the network's name, input matrix sizes and input data as arguments. 
	The object can load a neural network with message "read" saved in HDF5 format. The message with the message "data" followed by a list 
	of numbers and ending with mnessage "end" will trigger the prediction process. In response, the object will output a list of numbers 
	with indexies of the corresponding size as a prediction from the network. When deleted from the patch, the object must deactivate 
	the python server and free all the memory taken.

	Tornike Karchkhadze, tkarchkhadze@ucsd.edu
*/




#include "ext.h"
#include "ext_obex.h"

#include <time.h>


#include "shellapi.h"
#include <windows.h>


#include <iostream>
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"

#include <mutex>


#include<stdio.h>		/* for dumpping memroy to file */

#include "z_dsp.h"			// required for MSP objects


//using namespace std::placeholders;


#define ADDRESS "127.0.0.1" /* local host ip */
#define OUTPUT_BUFFER_SIZE 24576 /* maximum buffer size for osc send */

/* This following class is for controling threading and data flow to and from the python server */
/* we have this class because this is much more convinient to call its instances and call its functions this way */

class thread_control
{
	std::mutex mutex;
	std::condition_variable condVar;

public:
	thread_control()
	{}
	void notify() /* this will be used to notify threads that server have done processing */
	{
		condVar.notify_one();
	}
	void waitforit() /* this will be used to stop threads and wait for responces from server */
	{
		std::unique_lock<std::mutex> mlock(mutex);
		condVar.wait(mlock);
	}
	void lock() {
		mutex.lock();
	}
	void unlock() {
		mutex.unlock();
	}
};



// Data Structures
typedef struct _binaural_ns {
	//t_object	ob;

	t_pxobject ob;

	double * data_buffer;	 /* input data buffer for stroring incoming data that will be sent to server */

	long filling_index;		/* filling buffer index */

	char filename[MAX_PATH_CHARS]; /* Keras Network name saved as .h5 file */

	int verbose_flag; /* defines if extention will or won't output execution times and other information */

	SHELLEXECUTEINFO lpExecInfo; /* Open .h5 file handler */

	/* paralel thereads for osc listening and outputing */
	t_systhread		listener_thread;
	t_systhread		output_thread;

	/* comunication ports with pythin server */
	int PORT_SENDER; 
	int PORT_LISTENER; 

	/* comunication with OSC listener */
	float* prediction;


	bool* server_predicted;		/* flag if server has predicted */
	bool* server_ready;			/* flag if server is running and ready to receive data */
	bool* python_import_done;	/* flag if server iported chunk of data and is ready to receive next chunk */

	/* thread controls */
	thread_control* out_tread_control;		/* lock, unlock and notify routine for output thread */
	thread_control* server_control;			/* lock, unlock and notify routine for server start up thread */
	thread_control* python_import_control;	/* lock, unlock and notify routine for server's data importing thread */
	
	float position[7*4];	/* array for incoming view data of the object */
	int position_index;		/* writing index to the array so we know where to read */
	
} t_binaural_ns;


// Prototypes
t_binaural_ns* binaural_ns_new(t_symbol* s, long argc, t_atom* argv);

//void* binaural_ns_new(t_symbol* s, long argc, t_atom* argv);

void		binaural_ns_assist(t_binaural_ns* x, void* b, long m, long a, char* s);
void		binaural_ns_clear(t_binaural_ns* x);
void		binaural_ns_free(t_binaural_ns* x);

void		binaural_ns_read(t_binaural_ns* x, t_symbol* s);
void		binaural_ns_doread(t_binaural_ns* x, t_symbol* s, long argc, t_atom* argv);
void		binaural_ns_server(t_binaural_ns* x, long command);
void		binaural_ns_verbose(t_binaural_ns* x, long command);

void		binaural_ns_OSC_data_sender(t_binaural_ns* x, double** ins, int argc, char* argv[]);
void		binaural_ns_OSC_time_to_predict_sender(t_binaural_ns* x);
void		binaural_ns_OSC_filename_sender(t_binaural_ns* x, char *filename);

void		* binaural_ns_OSC_listener(t_binaural_ns* x, int argc, char* argv[]);
void		binaural_ns_OSC_listen_thread(t_binaural_ns* x);

void		binaural_ns_start_output_thread(t_binaural_ns* x);
void		binaural_ns_output_thread_stop(t_binaural_ns* x);
void		binaural_ns_output(t_binaural_ns* x);

void		binaural_ns_position(t_binaural_ns* x, t_symbol* s, long argc, t_atom* argv);

void		timestamp();


void		binaural_ns_dsp64(t_binaural_ns* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags);
void		binaural_ns_perform64(t_binaural_ns* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam);






// Globals and Statics
static t_class* s_binaural_ns_class = NULL;

///**********************************************************************/

// Class Definition and Life Cycle

void ext_main(void* r)
{
	t_class* c;

	c = class_new("binaural_ns~", (method)binaural_ns_new, (method)dsp_free, sizeof(t_binaural_ns), (method)NULL, A_GIMME, 0L);

	//t_class* c = class_new("binaural_ns~", (method)binaural_ns_new, (method)dsp_free, (long)sizeof(t_binaural_ns), 0L, A_GIMME, 0);

	
	class_addmethod(c, (method)binaural_ns_position, "position", A_GIMME, 0);

	class_addmethod(c, (method)binaural_ns_assist, "assist", A_CANT, 0);

	class_addmethod(c, (method)binaural_ns_read, "read", A_DEFSYM, 0);
	class_addmethod(c, (method)binaural_ns_server, "server", A_LONG, 0);
	class_addmethod(c, (method)binaural_ns_clear, "clear", 0);
	class_addmethod(c, (method)binaural_ns_verbose, "verbose", A_LONG, 0);

	class_addmethod(c, (method)binaural_ns_dsp64, "dsp64", A_CANT, 0);
	
	

	//class_addmethod(c, (method)binaural_ns_post, "post", 0);		/* this is for posting what's in buffer. used for debugging. this will desapiar */
	//class_addmethod(c, (method)binaural_ns_file, "save", 0);		/* this is for saving data buffer as file for debuging purposes. this will desapiar too */

	/* attributes */
	CLASS_ATTR_LONG(c, "verb", 0, t_binaural_ns, verbose_flag);

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	s_binaural_ns_class = c;
}


/***********************************************************************/
/***********************************************************************/
/********************** Initialisation *********************************/
/***********************************************************************/
/***********************************************************************/

t_binaural_ns* binaural_ns_new(t_symbol* s, long argc, t_atom* argv)
{
	t_binaural_ns* x = (t_binaural_ns*)object_alloc(s_binaural_ns_class);

	if (x) {

		dsp_setup((t_pxobject*)x, 1);	// MSP inlets: arg is # of inlets and is REQUIRED!
		// use 0 if you don't need inlets
		outlet_new(x, "signal"); 		// signal outlet L (note "signal" rather than NULL)
		outlet_new(x, "signal"); 		// signal outlet R (note "signal" rather than NULL)

		inlet_new(x, NULL);

		/* initialising variables */

		/* generate 2 random number that will beconme port numbers */
		srand(time(NULL));		 
		x->PORT_SENDER = rand() % 100000;	
		x->PORT_LISTENER = rand() % 100000;
		
		memset(x->filename, 0, MAX_PATH_CHARS); /* emptying network name variable for the begining */

		x->verbose_flag = 0;
		x->filling_index = 0;
		x->position_index = 0;
					
		//x->lpExecInfo = NULL;			/* no need to be initialised */

		/* Starting up OSC listener server and output thread */
		binaural_ns_OSC_listen_thread(x);   /* this function starts OSC listener inside sub-thread */
		//binaural_ns_start_output_thread(x); /* this starts up output thread that runs independently and outputs whenever server gives answer */
		
		x->out_tread_control = new thread_control;
		x->server_control = new thread_control;
		x->python_import_control = new thread_control;

		x->data_buffer = (double *)sysmem_newptr(sizeof(double) * 4096);
				
		/* here we check if argument are given */

		long offset = attr_args_offset((short)argc, argv); /* this is number of arguments before attributes start with @-sign */

		if (offset && offset > 0) {

			/* first argument is filename */
			binaural_ns_read(x, atom_getsym(argv));

		}

		attr_args_process(x, argc, argv); /* this is attribute reader */
		
	}
	return x;

}


void binaural_ns_free(t_binaural_ns* x)
{

	sysmem_freeptr(x->data_buffer);
	dsp_free((t_pxobject*)x);

	/************** Close pythoon server ****************/
	TerminateProcess(x->lpExecInfo.hProcess, 0);
	CloseHandle(x->lpExecInfo.hProcess);

}





// registers a function for the signal chain in Max
void binaural_ns_dsp64(t_binaural_ns* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags)
{
	post("my sample rate is: %f", samplerate);

	// instead of calling dsp_add(), we send the "dsp_add64" message to the object representing the dsp chain
	// the arguments passed are:
	// 1: the dsp64 object passed-in by the calling function
	// 2: the symbol of the "dsp_add64" message we are sending
	// 3: a pointer to your object
	// 4: a pointer to your 64-bit perform method
	// 5: flags to alter how the signal chain handles your object -- just pass 0
	// 6: a generic pointer that you can use to pass any additional data to your perform method

	object_method(dsp64, gensym("dsp_add64"), x, binaural_ns_perform64, 0, NULL);
}


// this is the 64-bit perform method audio vectors
void binaural_ns_perform64(t_binaural_ns* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam)
{
	t_double* inL = ins[0];		// we get audio for each inlet of the object from the **ins argument
	t_double* outL = outs[0];	// we get audio for each outlet of the object from the **outs argument
	t_double* outR = outs[1];	// we get audio for each outlet of the object from the **outs argument
	int n = sampleframes;

	if (*x->server_ready) {
		//post("star sending"); timestamp();
		binaural_ns_OSC_data_sender(x, ins, NULL, NULL);
		//post("done sending"); timestamp();

		binaural_ns_OSC_time_to_predict_sender(x);
		//post("predict flag sent"); timestamp();

		x->filling_index = 0;

		/* this calls threadcontroll class and stops this thread before
		notification from output thread come that 2048 sample are processed */
		//Sleep(35);

		while (x->filling_index < 4096) {

			x->out_tread_control->waitforit();

			if (x->server_predicted != nullptr && *x->server_predicted) {

				for (int i = 0; i < 512; i++) {

					if (x->filling_index < 2048) { 
						*(outL + x->filling_index) = (double)x->prediction[i];}
					else { *(outR + x->filling_index - 2048) = (double)x->prediction[i]; }

					x->filling_index++;

				}

				*x->server_predicted = false;

			}

		}

		//// this perform method simply copies the input to the output, offsetting the value
		//for (int i = 0; i < n; i++) {
		//	*outL++ = *(x->data_buffer + i);
		//	*outR++ = *(x->data_buffer + 2048 + i);
		//}

		//timestamp();

	}
	else {
		while (n--) {
			*outL++ = *outR++ = *inL++;
		}
	
	}

}







/**********************************************************************/
/**********************************************************************/
/*************************** Methods ********************************/
/**********************************************************************/
/**********************************************************************/


/* this function shows info when user brings mouse to inlets and outlets */
void  binaural_ns_assist(t_binaural_ns* x, void* b, long m, long a, char* s)
{
	if (m == ASSIST_INLET) { // inlet
		sprintf(s, "Mono sigal");
	}
	else {	// outlet
		switch (a) {
		case 0: sprintf(s, "Output Left"); break;
		case 1: sprintf(s, "Output Right"); break;
		}
	}
}

/************************ This function clears all the settings in object and server ******/
void binaural_ns_clear(t_binaural_ns* x) {

	x->filling_index = 0;
	x->out_tread_control->notify();
	*x->python_import_done = false;


	memset(x->filename, 0, MAX_PATH_CHARS); /* emptying array for the begining array */
	//*x->python_import_done = false;
	x->verbose_flag = 0;
	
	if (*x->server_ready) { /* if server is running it is restarted */
		binaural_ns_server(x, 0); /* close server */
		binaural_ns_server(x, 1); /* start server over */
	}
}

/* function that sets verbose flag. when werbose is 1 server outputs log data */
void binaural_ns_verbose(t_binaural_ns* x, long command) {

	x->verbose_flag = command;

	if (command == 1) {

		post("Network file: %s", x->filename);
		post("Listening on port: %d", x->PORT_LISTENER);
		post("Sending to port: %d", x->PORT_SENDER);
		post("Server running: %d", *x->server_ready);
	}
	else if (command == 0) {}

}



void binaural_ns_position(t_binaural_ns* x, t_symbol* s, long argc, t_atom* argv)
{
	// With each incoming agument with keyword "position" we fill an array.
	for (int i = 0; i < argc; i++) {

		*(x->position+ x->position_index *7 +i) = (float)atom_getfloat(argv+i);
		
	}

	x->position_index++;
	if (x->position_index > 3)
		x->position_index = 0;
}







/* creating output subthread */
void binaural_ns_start_output_thread(t_binaural_ns* x)
{
	if (x->output_thread == NULL) {

		//post("initialising new thread for output!"); timestamp();
		systhread_create((method)binaural_ns_output, x, 0, 0, 0, &x->output_thread);
	}
}


/******* output function that runs in independent thread and outputs data *******/
/********************** whenever it gets prediction from the server *************/
void binaural_ns_output(t_binaural_ns* x)
{	
	//char* p;
	//double f;
	x->filling_index = 0;
	
	while (true) {

		post("this should not be running");
		
		/* this calls threadcontroll class and stops this thread as soon as it starts. It waits for notification from output osc listener */
		x->out_tread_control->waitforit(); 

		//timestamp();

		if (x->server_predicted !=nullptr && *x->server_predicted) {

			for (int i = 0; i < 512; i++) {
			
				*(x->data_buffer + x->filling_index) = (double) x->prediction[i];

				x->filling_index++;
			}
					
		if (x->filling_index == 4096) { x->filling_index = 0; x->out_tread_control->notify();}
						
		*x->server_predicted = false;	/* reseting back server status for next prediction */
		}
	}
}


/* stoping output thread */
/* we ended up not useing this function but let it be here */
void binaural_ns_output_thread_stop(t_binaural_ns* x)
{
	unsigned int ret;

	if (x->output_thread) {
		post("stopping output thread, that must never happen :))");
		systhread_join(x->output_thread, &ret);					// wait for the thread to stop
		x->output_thread = NULL;
	}
}




/********************** read network *******************/
void binaural_ns_read(t_binaural_ns* x, t_symbol* s)
{
	defer((t_pxobject*)x, (method)binaural_ns_doread, s, 0, NULL);
}


void binaural_ns_doread(t_binaural_ns* x, t_symbol* s, long argc, t_atom* argv)
{
	short path, err;
	t_fourcc type; //= FOUR_CHAR_CODE('h5');
	char file[MAX_PATH_CHARS]; /*name of the file will be used to extract full path */

	if (s == gensym("")) { /* open diealog when no filename is given */
		if (open_dialog(file, &path, &type, &type, 1))
			return;
	}
	else { /* locating file when name is given */
		strcpy(file, s->s_name);
		if (locatefile_extended(file, &path, &type, NULL, 0)) {
			post("can't find file %s", file);
			return;
		}
	}
	err = path_toabsolutesystempath(path, file, x->filename); /* ablolute path */
	if (err) {
		post("%s: error %d opening file", file , err);
		return;
	}

	//post("%s", file);
	post("Network imported: %s", x->filename);

	/**********************************************************/
		/*  Osc send the network file name to server */
	/**********************************************************/
	binaural_ns_OSC_filename_sender(x, x->filename);

}



/****************************************************/
/******************* Python Server ******************/
/****************************************************/


static void binaural_ns_server(t_binaural_ns* x, long command)
{
	char text[512];

	/* this text will be pass to shell executer as argunents to set server port numbers  */
	snprintf(text, 512, "--serverport %d --clientport %d", x->PORT_SENDER, x->PORT_LISTENER);

	/******locating server.py file in max directories *********/

	short path, err;
	t_fourcc type;
	char file[MAX_PATH_CHARS] = "binaural_server.py";
	char fullpath[MAX_PATH_CHARS];

	if (locatefile_extended(file, &path, &type, NULL, 0)) {
		post("can't find file %s", file);
		return;
	}
	err = path_toabsolutesystempath(path, file, fullpath);

	/**********************************************************/

	x->lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	x->lpExecInfo.lpFile = fullpath; //"server.py";
	//x->lpExecInfo.lpFile = "AAAserver.py";
	x->lpExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	x->lpExecInfo.hwnd = NULL;
	x->lpExecInfo.lpVerb = NULL;
	x->lpExecInfo.lpParameters = text; //"--serverport 1000 --clientport 2000"; 

	//x->lpExecInfo.lpDirectory = "..\\..\\..\\..\\..\\..\\binaural_ns";
	x->lpExecInfo.lpDirectory = NULL; //"..\\..\\..\\..\\Max 8\\Packages\\max-sdk-main\\externals";   //Max 8\\Packages
	x->lpExecInfo.nShow = SW_SHOWNORMAL;
	
	
	if (command == 1) /* Opening the server */
	{
		ShellExecuteEx(&x->lpExecInfo);

		*x->server_ready = false;

		/* wait before python server ready flag becomes true */
		x->server_control->waitforit();
	
		/* if network was loaded we realod network too */
		if (x->filename[0]!=0) { binaural_ns_OSC_filename_sender(x, x->filename); }; 
	}
	else if (command == 0) {/* Closing the server */
		if (x->lpExecInfo.hProcess)
		{	
			*x->server_predicted = false;
			*x->server_ready = false;
			x->out_tread_control->notify(); /*this reaalises DSP block */
			
			
			TerminateProcess(x->lpExecInfo.hProcess, 0);
			CloseHandle(x->lpExecInfo.hProcess);
			*x->server_ready = false;

		}
		
	}
}



/****************************************** Some help and debuging functions **************************/

/* expoerting memoruy to text file for testing */

//void binaural_ns_file(t_binaural_ns* x)
//{
//	FILE* f = fopen("AAAmemory.txt", "w");
//	for (unsigned i = 0; i < 2048; i++) {
//		fprintf(f, "%f	", *(x->data_buffer + i));
//		
//	}
//
//	fclose(f);
//}

void timestamp()     /*********this is used to track timing of data flow *****/
{

	SYSTEMTIME t;
	GetSystemTime(&t); // or GetLocalTime(&t)
	post("%02d:%02d.%4d\n",
		t.wMinute, t.wSecond, t.wMilliseconds);

}
/***********************************************************************************************/



/**************************** OSC SERVER *****************************/



/*********************************************************************/
/*************************** Data Send *******************************/
/*********************************************************************/

/* function for data sending */
void binaural_ns_OSC_data_sender(t_binaural_ns* x, double** ins, int argc, char* argv[])
{
	(void)argc; // suppress unused parameter warnings
	(void)argv; // suppress unused parameter warnings


	UdpTransmitSocket transmitSocket(IpEndpointName(ADDRESS, x->PORT_SENDER));

	char buffer[OUTPUT_BUFFER_SIZE];

	osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);
	
	p.Clear();
	p << osc::BeginMessage("/data");

	for (int i = 0; i < 2048; i++) {
		p << *(*ins + i);
	}

	for (int k = 0; k < 4; k++) {
		
		int readindex = x->position_index + k;
		if (readindex > 3)
			readindex = readindex - 4;

		for (int i = 0; i < 7; i++) {

			p << (double)*(x->position + readindex * 7 + i);

		}
	}

	p << osc::EndMessage;

	transmitSocket.Send(p.Data(), p.Size());

	/* set python import flag to false */
	*x->python_import_done = false;

	
	/* wait before server responds and python import flag becomes true */
	x->python_import_control->waitforit();
		
}




/* function to send network file name */
void binaural_ns_OSC_filename_sender(t_binaural_ns* x, char* filename)
{

	UdpTransmitSocket transmitSocket(IpEndpointName(ADDRESS, x->PORT_SENDER));

	char buffer[4096];

	osc::OutboundPacketStream p(buffer, 4096);

	p << osc::BeginMessage("/filename") << filename << osc::EndMessage;


	transmitSocket.Send(p.Data(), p.Size());

}

/* send signal that server needs to predict now */
void binaural_ns_OSC_time_to_predict_sender(t_binaural_ns* x)
{
	int one = 1;

	UdpTransmitSocket transmitSocket(IpEndpointName(ADDRESS, x->PORT_SENDER));
	
	char buffer[32];

	osc::OutboundPacketStream p(buffer, 32);

	p << osc::BeginMessage("/predict") << one << osc::EndMessage;

	transmitSocket.Send(p.Data(), p.Size());

}







/************************************************************/
/************************* Listening ************************/
/************************************************************/


class binaural_ns_packetListener : public osc::OscPacketListener {

public:

	/* these are thread controls that are defined here and will be pluged to look at thread controls from the object */
	thread_control* out_tread_control;
	thread_control* server_control;
	thread_control* python_import_control;

	/* allocating buffer for output data */
	float prediction[512];


	/* these are local variables that will be aslo read form object variables */
	bool server_predicted=false;
	bool server_ready = false;
	bool python_import_done = false;

	void ProcessBundle(const osc::ReceivedBundle& b,
		const IpEndpointName& remoteEndpoint)
	{
		////ignore bundle time tag for now

		(void)remoteEndpoint; // suppress unused parameter warning

		for (osc::ReceivedBundle::const_iterator i = b.ElementsBegin();
			i != b.ElementsEnd(); ++i) {
			if (i->IsBundle())
				ProcessBundle(osc::ReceivedBundle(*i), remoteEndpoint);
			else
				ProcessMessage(osc::ReceivedMessage(*i), remoteEndpoint);   //, NULL
		}
	}




	virtual void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint)
	{
		(void)remoteEndpoint; // suppress unused parameter warning
		
		try {
			/* here comes signal that sevrer did append part of the data and needs next part, while sending data to server */
			if (std::strcmp(m.AddressPattern(), "/appended") == 0) {
				osc::ReceivedMessageArgumentStream args = m.ArgumentStream();

				/**** Here we read osc message with local variable. this is read from the main stuct also by pointer ***/
				python_import_control->lock();
				args >> python_import_done >> osc::EndMessage;
				python_import_control->unlock();

				/***** notify data sending function that is waiting for this ****/
				python_import_control->notify();
			}

			/* Here comes signal that server is loaded and running */
			if (std::strcmp(m.AddressPattern(), "/ready") == 0) {
				osc::ReceivedMessageArgumentStream args = m.ArgumentStream();

				/**** Here we read osc message with local variable. this is read from the main stuct also by pointer ***/

				server_control->lock();
				args >> server_ready >> osc::EndMessage;
				server_control->unlock();

				/***** notify main thread server starting function that is waiting for this ****/
				server_control->notify();
			}

			/* here comes prediction data in chunks of 512 */
			if (std::strcmp(m.AddressPattern(), "/prediction") == 0) {
				osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
			
				
				out_tread_control->lock();

				/**** Here we read osc message with local variable, that is read from main stuct by pointer ***/

				for (int i = 0; i < 512; i++) {
					args >> prediction[i];
				}

				server_predicted= true;

				out_tread_control->unlock();
								
				///***** notify output thread that is waiting for this ****/
				out_tread_control->notify();

			}

		}
		catch (osc::Exception& e) {

			post("error while parsing message from server!");

		}
	}

};

/* starting listerenr server thread */
void binaural_ns_OSC_listen_thread(t_binaural_ns* x)
{
	// create new thread + begin execution
	if (x->listener_thread == NULL) {

		systhread_create((method)binaural_ns_OSC_listener, x, 0, 0, 0, &x->listener_thread);
	}
}

/* this is actiual listerer server thread */
void *binaural_ns_OSC_listener(t_binaural_ns* x, int argc, char* argv[])
{
	(void)argc; // suppress unused parameter warnings
	(void)argv; // suppress unused parameter warnings

	binaural_ns_packetListener listener;

	UdpListeningReceiveSocket s(
		IpEndpointName(IpEndpointName::ANY_ADDRESS, x->PORT_LISTENER),
		&listener);

	post("Listening to port: %d", x->PORT_LISTENER);

	/* here we point struct variables to listener's local variables */
	x->prediction = listener.prediction;

	x->server_predicted = &listener.server_predicted;
	x->server_ready = &listener.server_ready;
	x->python_import_done = &listener.python_import_done;

	/* here same thing but other way around - listener's local class (and functions) point at struct class */
	listener.out_tread_control = x->out_tread_control;
	listener.server_control = x->server_control;
	listener.python_import_control = x->python_import_control;

	s.Run();	/* listerenr server will run forever! */

	return NULL;
}