// REQUIRES: example 
// test that it compiles, links and loads correctly.
// RUN: %bin/easyjit-example 1 1 2 3 4 s 1 2 3 4 s 1 2 3 4 q

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <numeric>
#include <iostream>
#include <sstream>
#include <sys/time.h>

// INLINE FROM HERE #INCLUDE_EASY#
#include <easy/jit.h>
// TO HERE #INCLUDE_EASY#

// INLINE FROM HERE #INCLUDE_EASY_CACHE#
#include <easy/code_cache.h>
// TO HERE #INCLUDE_EASY_CACHE#

struct timeval;

static double get_fps() {
  static struct timeval before;
  struct timeval now;

  gettimeofday(&now, nullptr);

  long secs = now.tv_sec - before.tv_sec;
  long usec = now.tv_usec - before.tv_usec;
  double diff = secs + ((double)usec)/1000000.0;
  before = now;

  return 1.0/diff;
}

namespace original {
// INLINE FROM HERE #ORIGINAL#
static void kernel(const char* mask, unsigned mask_size, unsigned mask_area,
                   const unsigned char* in, unsigned char* out,
                   unsigned rows, unsigned cols, unsigned channels) {
  unsigned mask_middle = (mask_size/2+1);
  unsigned middle = (cols+1)*mask_middle;

  for(unsigned i = 0; i != rows-mask_size; ++i) {
    for(unsigned j = 0; j != cols-mask_size; ++j) {
      for(unsigned ch = 0; ch != channels; ++ch) {

        long out_val = 0;
        for(unsigned ii = 0; ii != mask_size; ++ii) {
          for(unsigned jj = 0; jj != mask_size; ++jj) {
            out_val += mask[ii*mask_size+jj] * in[((i+ii)*cols+j+jj)*channels+ch];
          }
        }
        out[(i*cols+j+middle)*channels+ch] = out_val / mask_area;
      }
    }
  }
}

static void apply_filter(const char *mask, unsigned mask_size, unsigned mask_area, cv::Mat &image, cv::Mat *&out) {
  kernel(mask, mask_size, mask_area, image.ptr(0,0), out->ptr(0,0), image.rows, image.cols, image.channels());
}
// TO HERE #ORIGINAL#
}

namespace jit {
  using original::kernel;
// INLINE FROM HERE #EASY#
static void apply_filter(const char *mask, unsigned mask_size, unsigned mask_area, cv::Mat &image, cv::Mat *&out) {
  using namespace std::placeholders;

  auto kernel_opt = easy::jit(kernel, mask, mask_size, mask_area, _1, _2, image.rows, image.cols, image.channels());
  kernel_opt(image.ptr(0,0), out->ptr(0,0));
}
// TO HERE #EASY#
}

namespace cache {
  using original::kernel;
// INLINE FROM HERE #EASY_CACHE#
static void apply_filter(const char *mask, unsigned mask_size, unsigned mask_area, cv::Mat &image, cv::Mat *&out) {
  using namespace std::placeholders;

  static easy::Cache<> cache;
  auto const &kernel_opt = cache.jit(kernel, mask, mask_size, mask_area, _1, _2, image.rows, image.cols, image.channels());
  kernel_opt(image.ptr(0,0), out->ptr(0,0));
}
// TO HERE #EASY_CACHE#
}

static const char mask_no_filter[1] = {1};
static const char mask_gauss_3[9] = {1,1,1,1,1,1,1,1,1};
static const char mask_gauss_5[25] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static const char mask_gauss_7[49] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

static const char* masks[] = {
  mask_no_filter,
  mask_gauss_3,
  mask_gauss_5,
  mask_gauss_7
};

static unsigned mask_size[] = {
  1,
  3,
  5,
  7
};

static unsigned mask_area[] = {
  1,
  9,
  25,
  49
};

static void (*apply_filter)(const char*, unsigned, unsigned, cv::Mat &, cv::Mat *&) = original::apply_filter;

static char get_keystroke(bool test, char** argv, int argc) {
  if(!test)
    return cv::waitKey(1);
  
  // read the first character of each argument and use it as the key stroke
  static int key_count = 2;
  if(key_count >= argc)
    return 'q';

  char key = argv[key_count][0];

  key_count++;
  return key;
}

static bool get_next_frame(bool test, cv::Mat &frame) {
  if(test) {
    // return a black frame
    frame = cv::Mat(cv::Size(640, 480), CV_8UC3, cv::Scalar(0));
    return true;
  } else {
    // capture from the video camara
    static cv::VideoCapture video(0);

    if(!video.isOpened()) {
      std::cerr << "cannot open camara.\n";
      return false;
    }

    video.read(frame);
    return true;
  }
}

int main(int argc, char** argv) {
  bool test = 0;
  if(argc > 2)
    test = atoi(argv[1]);

  std::cerr << "\npress 1, 2, 3, 4 to change the filter.\n"
               "s to switch the implementation, from original to easy-jit(no cache), and to easy-jit(cache).\n"
               "q to exit.\n\n";

  int mask_no = 0;

  cv::Mat Frame;
  static cv::Mat Output;
  cv::Mat *Out = nullptr;

  unsigned time = 0;
  std::stringstream fps_message;

  double fps_history[4];

  while (true) {
    if(!get_next_frame(test, Frame))
      return -1;

    // allocate the output frame
    if(!Out) {
      Output = Frame.clone();
      Out = &Output;
    }

    get_fps();

    // apply the filter
    apply_filter(masks[mask_no], mask_size[mask_no], mask_area[mask_no], Frame, Out);

    double fps = get_fps();
    fps_history[time%4] = fps;

    if(time % 4 == 0) {
      double fps_avg = std::accumulate(std::begin(fps_history), std::end(fps_history), 0.0)/4.0;
      fps_message.str("");
      fps_message << "fps: " << fps_avg;
    }

    // show the fps, updated every 4 iterations
    cv::putText(*Out, fps_message.str().c_str(), cvPoint(30,30),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8,
                cvScalar(200,200,250), 1, CV_AA);

    if(!test) {
      cv::imshow("camera", *Out);
    } 
    ++time;

    // user input
    char key = get_keystroke(test, argv, argc);
    if (key < 0)
      continue;

    switch(key) {
      case 'q':
        return 0;
      case 's':
      {
        if(apply_filter == original::apply_filter) {
          std::cerr << "using easy::jit (no-cache) implementation\n";
          apply_filter = jit::apply_filter;
        } else if (apply_filter == jit::apply_filter) {
          std::cerr << "using easy::jit (cache) implementation\n";
          apply_filter = cache::apply_filter;
        } else {
          std::cerr << "using original implementation\n";
          apply_filter = original::apply_filter;
        }
        break;
      }
      case '1':
      case '2':
      case '3':
      case '4':
      {
        mask_no = key-'1';
        std::cerr << "change mask to " << mask_no+1 << "!\n";
        break;
      }
      default:
        std::cerr << "unknown stroke " << key << "\n";
    }
  }

  return 0;
}
