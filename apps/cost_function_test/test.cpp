#include <cmath>
#include <cstdint>
#include <cstdio>

#include "benchmark.h"
#include "halide_image.h"
#include "cost.h"

using namespace Halide::Tools;

//#define cimg_display 0
//#include "CImg.h"
//using namespace cimg_library;

// typedef CImg<uint16_t> Image;

double t;

Image<uint16_t> cost_function_test(Image<uint16_t> in) {
    Image<uint16_t> out(in.width()-208, in.height()-202);

    // Call it once to initialize the halide runtime stuff
    cost(in, out);

    t = benchmark(10, 1, [&]() {
        // Compute the same region of the output as blur_fast (i.e., we're
        // still being sloppy with boundary conditions)
        cost(in, out);
    });

    return out;
}

int main(int argc, char **argv) {

    Image<uint16_t> input(6408, 4802);

    for (int y = 0; y < input.height(); y++) {
        for (int x = 0; x < input.width(); x++) {
            input(x, y) = rand() & 0xfff;
        }
    }

    Image<uint16_t> halide = cost_function_test(input);
    double halide_time = t;

    // fast_time2 is always slower than fast_time, so skip printing it
    //printf("times: %f %f %f\n", slow_time, fast_time, halide_time);
    printf("runtime: %g\n", halide_time * 1000);


    return 0;
}
