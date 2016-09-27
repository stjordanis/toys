#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>

#include <algorithm>
#include "avx512-sort.cpp"

void print(const char* s) {
    printf("%s... ", s);
    fflush(stdout);
}



class Failed {};


template <unsigned N>
class TestBase {

    static_assert(N == 16 || N == 32, "N has got invalid value");

protected:

    uint32_t in[N];
    uint32_t out[N];
    uint32_t ref[N];

    virtual void sort() = 0;

public:
    void run() {

        print("test ascending");
        input_ascending();
        check();
        puts("OK");

        print("test descending");
        input_descending();
        check();
        puts("OK");

        print("test all same");
        input_all_same();
        check();
        puts("OK");

        print("test random");
        for (int i=0; i < 1000; i++) {
            input_random();
            check();
        }
    }

private:

    void input_ascending() {
        for (int i=0; i < N; i++) {
            in[i] = i;
        }
    }

    void input_descending() {
        for (int i=0; i < N; i++) {
            in[i] = N-i;
        }
    }

    void input_all_same() {
        for (int i=0; i < N; i++) {
            in[i] = 42;
        }
    }

    void input_random() {
        for (int i=0; i < N; i++) {
            in[i] = rand();
        }
    }

    void check() {

        // run an AVX512 procedure
        sort();

        // run reference
        memcpy(ref, in, sizeof(in));
        std::sort(ref, ref + N);

        // compare
        for (int i=1; i < N; i++) {
            if (ref[i] != out[i]) {
                printf("mismatch at %d\n", i);
                dump("in ", in);
                dump("out", out);
                throw Failed();
            }
        }
    }

    void dump(const char* name, const uint32_t* data) {

        printf("%s = [", name);
        for (int i=0; i < N; i++) {
            if (i > 0) printf(", ");
            printf("%5d", data[i]);
        }
        printf("]\n");
    }
};


class Test1Reg: public TestBase<16> {

public:
    using FunctionPtr = __m512i (*)(const __m512i v);

private:
    FunctionPtr function;

public:
    Test1Reg(FunctionPtr fn) : function(fn) {}

private:
    virtual void sort() override {

        const __m512i input  = _mm512_loadu_si512(in);
        const __m512i sorted = function(input);
        _mm512_storeu_si512(out, sorted);
    }
};


class Test2Regs: public TestBase<32> {

public:
    using FunctionPtr = void (*)(const __m512i v1, const __m512i v2, __m512i& r1, __m512i& r2);

private:
    FunctionPtr function;

public:
    Test2Regs(FunctionPtr fn) : function(fn) {}

private:
    virtual void sort() override {

        // run an AVX512 procedure
        const __m512i in1  = _mm512_loadu_si512(in);
        const __m512i in2  = _mm512_loadu_si512(in + 16);
        __m512i sorted1, sorted2;

        function(in1, in2, sorted1, sorted2);

        _mm512_storeu_si512(out, sorted1);
        _mm512_storeu_si512(out + 16, sorted2);
    }
};


int main() {

    {
        puts("");
        puts("avx512_sort2xreg_epi32");
        Test2Regs test(avx512_sort2xreg_epi32);

        try {
            test.run();
            puts("OK");
        } catch (Failed&) {
            puts("ERROR");
            return 1;
        }
    }

    {
        puts("");
        puts("avx512_sort_epi32");
        Test1Reg test(avx512_sort_epi32);

        try {
            test.run();
            puts("OK");
        } catch (Failed&) {
            puts("ERROR");
        }
    }

    {
        puts("");
        puts("avx512_sort_epi32_unrolled");
        Test1Reg test(avx512_sort_epi32_unrolled);

        try {
            test.run();
            puts("OK");
        } catch (Failed&) {
            puts("ERROR");
        }
    }

    {
        puts("");
        puts("avx512_sort_loop_epi32");
        Test1Reg test(avx512_sort_loop_epi32);

        try {
            test.run();
            puts("OK");
        } catch (Failed&) {
            puts("ERROR");
        }
    }

    {
        puts("");
        puts("avx512_sort_while_epi32");
        Test1Reg test(avx512_sort_while_epi32);

        try {
            test.run();
            puts("OK");
        } catch (Failed&) {
            puts("ERROR");
        }
    }

    {
        puts("");
        puts("avx512_sort2xreg_epi32");
        Test2Regs test(avx512_sort2xreg_epi32);

        try {
            test.run();
            puts("OK");
        } catch (Failed&) {
            puts("ERROR");
        }
    }
}