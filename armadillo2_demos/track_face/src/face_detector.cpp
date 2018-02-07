
#include <face_detector.h>

FaceDetector::FaceDetector()
{
    if( !face_cascade.load( FACE_DATA_PATH ) || !eyes_cascade.load( EYES_DATA_PATH ))
    {
        ROS_ERROR("[face_detector]: error loading data file");
        exit(EXIT_FAILURE);
    }

    cap_.open(0);
    if (!cap_.isOpened())
    {
        ROS_ERROR("[face_detector]: error opening camera stream");
        exit(EXIT_FAILURE);
    }

}

bool FaceDetector::detectAndDisplay(CvPoint& face_point, cv::Rect& frame_data)
{
    cv::Mat frame;
    cap_ >> frame;

    if (frame.empty())
        return false;

    std::vector<cv::Rect> faces, eyes;
    cv::Mat frame_gray;

    cvtColor( frame, frame_gray, CV_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );

    /* detect faces */
    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30, 30) );

    float biggest_face_size = 0;
    int biggest_face_indx = 0;

    for (int face_indx = 0; face_indx < faces.size(); face_indx++)
    {
        if (faces[face_indx].empty())
            return false;

        cv::Mat faceROI = frame_gray( faces[face_indx] );

        //-- In each face, detect eyes
        eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, cv::Size(30, 30) );

        // draw faces with eyes (reduce false positives)
        if (eyes.size() > 0)
        {
            face_point.x = faces[face_indx].x +  faces[face_indx].width / 2;
            face_point.y = faces[face_indx].y + faces[face_indx].height / 2;
            frame_data.width = frame.cols;
            frame_data.height = frame.rows;

            rectangle(frame, CvPoint(faces[face_indx].x, faces[face_indx].y),
                      CvPoint(faces[face_indx].x + faces[face_indx].width,
                              faces[face_indx].y + faces[face_indx].height), 2, 8, 0 );

            imshow(DETECTION_WINDOW_NAME, frame );

            return true;
        }
        // if no eyes were found in faces, maby face is far away, so draw biggest face
        if (faces[face_indx].area() > biggest_face_size)
        {
            biggest_face_size = faces[face_indx].area();
            biggest_face_indx = face_indx;

            if (face_indx == faces.size() - 1)
            {
                face_point.x = faces[biggest_face_indx].x;
                face_point.y = faces[biggest_face_indx].y;
                frame_data.width = frame.cols;
                frame_data.height = frame.rows;

                rectangle(frame, CvPoint(faces[biggest_face_indx].x, faces[biggest_face_indx].y),
                          CvPoint(faces[biggest_face_indx].x + faces[biggest_face_indx].width,
                                  faces[biggest_face_indx].y + faces[biggest_face_indx].height), 2, 8, 0 );

                imshow(DETECTION_WINDOW_NAME, frame );
                return true;
            }
        }

        face_indx++;
    }

    return false;

}

double FaceDetector::map(double input_start,
                           double input_end,
                           double output_start,
                           double output_end,
                           double input)
{
    return output_start + ((output_end - output_start) / (input_end - input_start)) * (input - input_start);
}