#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;
using namespace std;

Mat src; Mat src_gray;
int thresh = 20;
int max_thresh = 255;   
RNG rng(12345);

int morph_elem = 0;
int morph_size = 0;
int morph_operator = 0;

/// Function header
void thresh_callback(int, void* );

/** @function main */
int main( int argc, char** argv )
{
    /// Load source image
    src = imread("test.jpg", 1 );
    imshow("Source", src);
    
    /// Convert image to gray
    cvtColor( src, src_gray, CV_BGR2GRAY );
    imshow("Source Gray",src_gray);
    
    // sort out perspective here
    
    Mat dstPerspective(src.rows,src.cols,CV_8UC1,Scalar::all(0)); //create destination image
    
    
    // threshold by binary_inv
    
    Mat ThresholdPerspective;
    threshold(src_gray, ThresholdPerspective, 70, 255, CV_THRESH_BINARY_INV);
    
    // get all the contours
    
    vector< vector <Point> > contoursPerspective; // Vector for storing contour
    vector< Vec4i > hierarchyPerspective;
    
    findContours( ThresholdPerspective, contoursPerspective, hierarchyPerspective,CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE ); // Find the contours in the image
    
    // find the largest contour
    
    int largest_contour_index=0;
    int largest_area=0;
    
    cout << "Starting\n";
    
    for( int i = 0; i< contoursPerspective.size(); i++ ){
        
        approxPolyDP(Mat(contoursPerspective[i]),contoursPerspective[i],10,true);
        
        if(contoursPerspective[i].size()==4){
            double a=contourArea( contoursPerspective[i],false);  //  Find the area of contour
            if(a>largest_area){
                largest_area=a;
                largest_contour_index=i;                //Store the index of largest contour
            }
        }
    }
    
    vector<vector<Point> > contours_poly(1);
    
    contours_poly[0] = contoursPerspective[largest_contour_index];
    cout << "Largest contour index is: " << largest_contour_index << "\n";
    
    Rect boundRect=boundingRect(contoursPerspective[largest_contour_index]);
    cout << contours_poly[0].size();
    if(contours_poly[0].size()==4){
        cout << "found a thing/n";
        std::vector<Point2f> quad_pts;
        std::vector<Point2f> squre_pts;
        
        
        
        quad_pts.push_back(Point2f(contours_poly[0][0].x,contours_poly[0][0].y));
        quad_pts.push_back(Point2f(contours_poly[0][1].x,contours_poly[0][1].y));
        quad_pts.push_back(Point2f(contours_poly[0][3].x,contours_poly[0][3].y));
        quad_pts.push_back(Point2f(contours_poly[0][2].x,contours_poly[0][2].y));
        
        // This rectangle should be the right size (6" x 0.75" x some factor to boost resolution) - Output that factor
        
        double length = cv::norm(quad_pts[0]-quad_pts[1]);
        double length2 = cv::norm(quad_pts[1]-quad_pts[3]);
        
        cout << "\nlength " << length << "\nlength2 " << length2 << "\nquad_pts[0] " << quad_pts[0] << "\nquad_pts[1] " << quad_pts[1] << "\naspectratio " << length/length2;
        
        length2 = length/8;
        
        // Reference ruler is 6 inches long = 152.4 mm.
        
        cout << "'\nPixels/mm = " << length / 152.4;
        
        squre_pts.push_back(Point2f(boundRect.x,boundRect.y));
        squre_pts.push_back(Point2f(boundRect.x,boundRect.y+length));
        squre_pts.push_back(Point2f(boundRect.x+length2,boundRect.y));
        squre_pts.push_back(Point2f(boundRect.x+length2,boundRect.y+length));
        
        Mat transmtx = getPerspectiveTransform(quad_pts,squre_pts);
        Mat transformed = Mat::zeros(src.rows, src.cols, CV_8UC3);
        
        
        warpPerspective(src, src_gray, transmtx, src.size());
        
    }
    
    imshow("src_gray and xfmd",src_gray);
    
    // ehhh, 'close' it with a morph size of 2?
    
    morph_size = 2;
    
    Mat element = getStructuringElement( 2, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );
    
    Mat src_gray_morph;
    morphologyEx( src_gray, src_gray_morph, 0, element );
    
    imshow("Source Gray Morph", src_gray_morph);
    
    // blur image 3x3 pixels
    
    Mat src_gray_morph_blur;
    blur( src_gray_morph, src_gray_morph_blur, Size(3,3) );
    
    // make variables
    
    Mat canny_output;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    
    
    // Detect edges using canny or threshold and get the outside edge (badness)
    
    //   Canny( src_gray_morph_blur, canny_output, thresh, thresh*2, 3 );
    cvtColor( src_gray_morph_blur, src_gray_morph_blur, CV_BGR2GRAY );
    
    threshold(src_gray_morph_blur, canny_output, 150,255,THRESH_BINARY_INV);
    
    imshow("Canny output",canny_output);
    
    /// Find contours
    findContours( canny_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
    
    // Make somewhere to put the output
    Mat drawing = Mat::zeros( canny_output.size(), CV_8UC3 );
    
    // Sort contours by size
    sort(contours.begin(), contours.end(), [](const vector<Point>& c1, const vector<Point>& c2){
        return contourArea(c1, false) < contourArea(c2, false);
    });
    
    // Loop over all the contours, ending with the largest
    int i=0;
    
    for (i = 0; i<contours.size(); i++)
    {
        
        // Pick a random colour
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        
        // Draw the contours filled
        drawContours( drawing, contours, i, color, -1, 8, hierarchy, 0, Point() );
        
    }
    
    imshow("Contours",drawing);
    imwrite("output.bmp",drawing);
    
    waitKey(0);
    return(0);
}


