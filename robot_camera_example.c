#include "robot_if.h"
#include "robot_color.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argv, char **argc) {
	robot_if_t ri;
	int major, minor;
	IplImage *image = NULL, *hsv = NULL, *threshold = NULL;
	squares_t *squares, *biggest, *sq_idx;
	CvPoint pt1, pt2;
	int sq_amt;

	// Make sure we have a valid command line argument
	if(argv <= 1) {
		printf("Usage: robot_test <address of robot>\n");	
		exit(-1);
	}

	ri_api_version(&major, &minor);
	printf("Robot API Test: API Version v%i.%i\n", major, minor);

	// Setup the robot with the address passed in
	if(ri_setup(&ri, argc[1], 0)) {
		printf("Failed to setup the robot!\n");
		exit(-1);
	}

	// Setup the camera
	if(ri_cfg_camera(&ri, RI_CAMERA_DEFAULT_BRIGHTNESS, RI_CAMERA_DEFAULT_CONTRAST, 5, RI_CAMERA_RES_640, RI_CAMERA_QUALITY_HIGH)) {
		printf("Failed to configure the camera!\n");
		exit(-1);
	}

	// Create a window to display the output
	cvNamedWindow("Rovio Camera", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("Biggest Square", CV_WINDOW_AUTOSIZE);

	// Create an image to store the image from the camera
	image = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);

	// Create an image to store the HSV version in
	// We configured the camera for 640x480 above, so use that size here
	hsv = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);

	// And an image for the thresholded version
	threshold = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 1);

	// Move the head up to the middle position
	ri_move(&ri, RI_HEAD_MIDDLE, RI_FASTEST);
	
	// Action loop
	do {
		// Update the robot's sensor information
		if(ri_update(&ri) != RI_RESP_SUCCESS) {
			printf("Failed to update sensor information!\n");
			continue;
		}

		// Get the current camera image and display it
		if(ri_get_image(&ri, image) != RI_RESP_SUCCESS) {
			printf("Unable to capture an image!\n");
			continue;
		}
		cvShowImage("Rovio Camera", image);

		// Convert the image from RGB to HSV
		cvCvtColor(image, hsv, CV_BGR2HSV);

		// Pick out only the yellow color from the image
		cvInRangeS(hsv, RC_PINK_LOW, RC_PINK_HIGH, threshold);

		// Find the squares in the image
		squares = ri_find_squares(threshold, RI_DEFAULT_SQUARE_SIZE);

		// Loop over the squares and find the biggest one
		biggest = squares;
		sq_idx = squares;
		while(sq_idx != NULL) {
			if(sq_idx->area > biggest->area)
				biggest = sq_idx;
			sq_idx = sq_idx->next;
		}
		
		// Only draw if we have squares
		if(biggest != NULL) {
			// Draw an X marker on the image
			sq_amt = (int) (sqrt(biggest->area) / 2);	

			// Upper Left to Lower Right
			pt1.x = biggest->center.x - sq_amt;
			pt1.y = biggest->center.y - sq_amt;
			pt2.x = biggest->center.x + sq_amt;
			pt2.y = biggest->center.y + sq_amt;
			cvLine(image, pt1, pt2, CV_RGB(0, 255, 0), 3, CV_AA, 0);

			// Lower Left to Upper Right
			pt1.x = biggest->center.x - sq_amt;
			pt1.y = biggest->center.y + sq_amt;
			pt2.x = biggest->center.x + sq_amt;
			pt2.y = biggest->center.y - sq_amt;
			cvLine(image, pt1, pt2, CV_RGB(0, 255, 0), 3, CV_AA, 0);
		}

		// Display the image with the drawing on it
		cvShowImage("Biggest Square", image);

		// Update the UI (10ms wait)
		cvWaitKey(10);

		// Release the square data
		while(squares != NULL) {
			sq_idx = squares->next;
			free(squares);
			squares = sq_idx;	
		}

		// Move forward unless there's something in front of the robot
		if(!ri_IR_Detected(&ri))
			ri_move(&ri, RI_MOVE_FORWARD, RI_SLOWEST);

	} while(1);

	// Clean up (although we'll never get here...)
	cvDestroyWindow("Rovio Camera");
	cvDestroyWindow("Biggest Square");
	
	// Free the images
	cvReleaseImage(&hsv);
	cvReleaseImage(&threshold);
	cvReleaseImage(&image);

	return 0;
}
