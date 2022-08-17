
// std
#include <iostream>
// opencv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

Mat_<Vec3b> lightFieldArray[17][17];
Mat_<Vec2f> uvValues;
Vec2f uv_centre;
int rows;
int cols;

float focus = 1.04; 
//0.7 Yellow
//0.73 Green
//0.78 Blue
//0.9 lightBlue
//1.04 Pink
float aperture = 30;

int posX = 8;
int posY = 8;

void display(string img_display, Mat m) {
	namedWindow(img_display, WINDOW_AUTOSIZE);
	imshow(img_display, m);
	waitKey(10);
}

void loadImages(String files) {
	// parse all images
	cout << "Loading light field ..." << std::endl;
	vector<String> lf_imgs;
	uvValues = Mat_<Vec2f>(17, 17);
	glob(files, lf_imgs);
	for (String cv_str : lf_imgs) {
		// get the filepath
		string filepath(cv_str);
		size_t pos = filepath.find_last_of("/\\");
		if (pos != std::string::npos) {
			// replace "_" with " "
			string filename = filepath.substr(pos + 1);
			replace(filename.begin(), filename.end(), '_', ' ');
			// parse for values
			istringstream ss(filename);
			string name;
			int row, col;
			float v, u;
			ss >> name >> row >> col >> v >> u;
			if (ss.good()) {
				lightFieldArray[row][col] = imread(filepath, 1);
				uvValues(row, col) = Vec2f(u, v);
				continue;

			}
		}
		// throw error otherwise
		std::cerr << "Filepath error with : " << filepath << std::endl;
		std::cerr << "Expected in the form : "
			"[prefix]/[name]_[row]_[col]_[v]_[u][suffix]";
		abort();

	}
	std::cout << "Finished loading light field" << std::endl;
	uv_centre = uvValues(8, 8);
	rows = lightFieldArray[0][0].rows;
	cols = lightFieldArray[0][0].cols;
}

Vec3i getLightField(int r, int c, int t, int s) {
	return lightFieldArray[r][c](t, s);
}

void get4DPoint(int r, int c, int t, int s) {
	cout << endl;
	cout << "4D Point for r = " << r << " c = " << c << " t = " << t << " s = " << s << endl;
	cout << lightFieldArray[r][c](t, s);
}

inline float ssd(Vec2f a, Vec2f b) {
	return ((a[0] - b[0]) * (a[0] - b[0])) + ((a[1] - b[1]) * (a[1] - b[1]));
}

inline float cityBlock(Vec2f a, Vec2f b) {
	return abs(a[0] - b[0]) + abs(a[1] - b[1]);
}


Mat_<Vec3i> generateSTArray(int s_start, int t_start, int rows, int cols, int radius) {
	Mat_<Vec3b> image = Mat_<Vec3b>(17 * rows, 17 * cols);
	radius = radius * radius;

	for (int t = 0; t < rows; t++) {
		for (int s = 0; s < cols; s++) {
			for (int i = 0; i < 17; i++) {
				for (int j = 0; j < 17; j++) {
					Vec3i p = { 0,0,0 };
					float r2 = ssd(uv_centre, uvValues(i, j));
					if (r2 < radius)
						p = lightFieldArray[i][j](t + t_start, s + s_start);
					image(t * 17 + i, s * 17 + (16 - j)) = p;

				}

			}

		}

	}
	return image;
}

Mat_<Vec3b> refocusSquare() {
	// this comes from page 53 of https://people.eecs.berkeley.edu/~ren/thesis/renng-thesis.pdf
	float fAlpha = 1 - (1 / focus);
	Mat_<Vec3b> image = Mat_<Vec3b>(rows, cols);
	for (int t = 0; t < rows; t++) {
		for (int s = 0; s < cols; s++) {
			Vec3i pixel = { 0,0,0 };
			int count = 0;
			for (int i = 0; i < 17; i++) {
				for (int j = 0; j < 17; j++) {

					Vec2f uvDist = uv_centre - uvValues(i, j);
					float r2 = cityBlock(uv_centre, uvValues(i, j));
					if (r2 < aperture) {
						int nt = t + uvDist[1] * fAlpha;
						int ns = s + uvDist[0] * fAlpha;
						if (nt >= 0 && nt < rows && ns >= 0 && ns < cols) {
							pixel = pixel + Vec3i(lightFieldArray[i][j](nt, ns));
							count++;
						}
					}

				}

			}
			pixel = pixel / count;
			image(t, s) = pixel;
		}
	}
	return image;
}

Mat_<Vec3b> refocus() {
	uv_centre = uvValues[posY][posX];
	// this comes from page 53 of https://people.eecs.berkeley.edu/~ren/thesis/renng-thesis.pdf
	float fAlpha = 1 - (1 / focus);
	Mat_<Vec3b> image = Mat_<Vec3b>(rows,cols);
	for (int t = 0; t < rows; t++) {
		for (int s = 0; s < cols; s++) {
			Vec3i pixel = { 0,0,0 };
			int count = 0;
			for (int i = 0; i < 17; i++) {
				for (int j = 0; j < 17; j++) {

					Vec2f uvDist = uv_centre - uvValues(i, j);
					float r2 = ssd(uv_centre, uvValues(i, j));
					if (r2 < aperture*aperture) {
						int nt = t + uvDist[1] * fAlpha;
						int ns = s + uvDist[0] * fAlpha;
						if (nt >= 0 && nt < rows && ns >= 0 && ns < cols) {
							pixel = pixel + Vec3i(lightFieldArray[i][j](nt, ns));
							count++;
						}
					}

				}

			}
			pixel = pixel / count;
			image(t, s) = pixel;
		}
	}
	return image;
}

void on_trackbar(int tbval, void* vp) {
	focus = 0.6f + tbval / 200.0f;
	Mat f1 = refocus();
	imshow("Refocus", f1);

}

void on_trackbarA(int tbval, void* vp) {
	aperture = 1 + tbval;
	Mat f1 = refocus();
	imshow("Refocus", f1);

}


void on_trackbarX(int tbval, void* vp) {
	posX = (tbval * 16)/100;
	Mat f1 = refocus();
	imshow("Refocus", f1);

}

void on_trackbarY(int tbval, void* vp) {
	posY = (tbval * 16) / 100;
	Mat f1 = refocus();
	imshow("Refocus", f1);

}

// main program
// 
int main( int argc, char** argv ) {

	loadImages("res/jellyBean/*.png");
	display("LightFieldDisplay", lightFieldArray[16][16]);
	display("Refocus", refocus());

	Mat st = generateSTArray(770, 205, 100, 100, 75);
	imwrite("Output_st_75.png", st);
	st = generateSTArray(770, 205, 100, 100, 40);
	imwrite("Output_st_40.png", st);
	bool drawFive = true;
	if (drawFive) {
		focus = 0.7;
		display("Focus: 0.7", refocus());
		focus = 0.73;
		display("Focus: 0.73", refocus());
		focus = 0.78;
		display("Focus: 0.78", refocus());
		focus = 0.9;
		display("Focus: 0.9", refocus());
		focus = 1.04;
		display("Focus: 1.04", refocus());
	}

	createTrackbar("Focus", "Refocus", nullptr , 100, on_trackbar);
	createTrackbar("Aperture", "Refocus", nullptr, 100, on_trackbarA);
	setTrackbarPos("Focus", "Refocus", 50);
	setTrackbarPos("Aperture", "Refocus", 50);

	createTrackbar("X Position", "Refocus", nullptr, 100, on_trackbarX);
	createTrackbar("Y Position", "Refocus", nullptr, 100, on_trackbarY);
	setTrackbarPos("X Position", "Refocus", 50);
	setTrackbarPos("Y Position", "Refocus", 50);
	get4DPoint(7, 10, 384, 768);

	// wait for a keystroke in the window before exiting
	waitKey(0);
}
