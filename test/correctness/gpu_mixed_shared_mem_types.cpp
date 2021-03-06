#include "Halide.h"
#include <stdio.h>

using namespace Halide;

template <typename T>
int check_result(Buffer &output_buffer, int n_types, int offset) {
    Image<T> output(output_buffer);
    for (int x = 0; x < output.width(); x++) {
        T correct = n_types * (static_cast<uint16_t>(x) / 16) + offset;
        if (output(x) != correct) {
            printf("output(%d) = %d instead of %d\n",
                   (unsigned int)x, (unsigned int)output(x), (unsigned int)correct);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    Target t(get_jit_target_from_environment());
    if (!t.has_gpu_feature()) {
        printf("Not running test because no gpu target enabled\n");
        return 0;
    }

    const int n_types = 9;

    Type types[] = {Int(8), Int(16), Int(32), Int(64),
                    UInt(8), UInt(16), UInt(32), UInt(64),
                    Float(32)};
    Func funcs[n_types];

    Var x;

    Func out;

    Type result_type;
    if (t.has_feature(Target::Metal)) {
        result_type = UInt(32);
    } else {
        result_type = UInt(64);
    }
    Expr e = cast(result_type, 0);
    int offset = 0;
    for (int i = 0; i < n_types; i++) {
        int off = 0;
        if ((types[i].is_int() || types[i].is_uint())) {
            // Metal does not support 64-bit integers.
            if (t.has_feature(Target::Metal) &&
                types[i].bits() >= 64) {
                continue;
            }

            if (types[i].bits() <= 64) {
                off = (1 << (types[i].bits() - 4)) + 17;
            }
        }
        offset += off;

        funcs[i](x) = cast(types[i], x/16 + off);
        e += cast(result_type, funcs[i](x));
        funcs[i].compute_at(out, Var::gpu_blocks()).gpu_threads(x);
    }


    out(x) = e;
    out.gpu_tile(x, 23);

    Buffer output = out.realize(23*5);

    int result;
    if (t.has_feature(Target::Metal)) {
        result = check_result<uint32_t>(output, n_types - 2, offset);
    } else {
        result = check_result<uint64_t>(output, n_types, offset);
    }
    if (result != 0) {
        return result;
    }

    printf("Success!\n");
    return 0;
}
