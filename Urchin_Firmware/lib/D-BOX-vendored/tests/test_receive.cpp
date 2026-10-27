// Host-side test of the ESP32 D-BOX receive() using a simulated UART.
// Bytes "arrive" over simulated time (1 byte per 87 us ~= 115200 baud).
// wait(ms) advances the clock; every rawread() call costs 1 us of CPU.
#include "DBOXprotocol.h"
#include <cstdio>
#include <deque>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
using namespace DBOXProtocol;

struct SpinGuard : std::runtime_error { using std::runtime_error::runtime_error; };

class SimUart : public IPort {
public:
    std::vector<uint8_t> stream; size_t next = 0;
    double now_us = 0, us_per_byte = 87.0;
    long rawreads = 0, waits = 0, rawreads_since_wait = 0, max_rawreads_without_wait = 0;
    long limit = 2'000'000;
    bool trickle = false;   // false: everything already arrived; true: arrives over time
    bool open() override { return true; }
    void close() override {}
    bool isOpen() const override { return true; }
    void flushBuss() override {}
    std::vector<std::string> find() override { return {}; }
    void wait(uint32_t ms) override { waits++; now_us += ms * 1000.0; rawreads_since_wait = 0;
        if (waits > 100000) throw SpinGuard("waited forever: no more data"); }
protected:
    std::size_t rawwrite(std::span<const uint8_t>, std::size_t s) override { return s; }
    std::size_t rawread(std::span<uint8_t> buf, std::size_t size) override {
        rawreads++; rawreads_since_wait++; now_us += 1.0;
        if (rawreads_since_wait > max_rawreads_without_wait) max_rawreads_without_wait = rawreads_since_wait;
        if (rawreads > limit) throw SpinGuard("stuck: over 2,000,000 checks");
        // trickle: first 3 bytes are already here, then the sender pauses 50 ms
        // (e.g. the Pi got busy) before the rest of the label arrives.
        size_t arrived = stream.size();
        if (trickle) arrived = now_us < 50000.0 ? std::min<size_t>(3, stream.size())
                                               : std::min(stream.size(), 3 + (size_t)((now_us - 50000.0) / us_per_byte) + 1);
        size_t n = 0;
        while (n < size && n < buf.size() && next < arrived) buf[n++] = stream[next++];
        return n;
    }
};

static std::vector<uint8_t> frame(uint8_t id, char st, std::string payload) {
    DBOX p(id, (uint8_t)st, payload);
    DBOXraw r(p);
    return static_cast<std::vector<uint8_t>>(r);
}
static void add(std::vector<uint8_t>& v, const std::vector<uint8_t>& x) { v.insert(v.end(), x.begin(), x.end()); }

struct Expect { uint8_t id; char st; std::string payload; };

static bool run_inner(const char* name, std::vector<uint8_t> bytes, std::vector<Expect> exp, bool trickle) {
    SimUart port; port.stream = bytes; port.trickle = trickle;
    DBOXprotocol proto(&port, "/dev/null");
    std::string why; bool ok = true;
    try {
        for (auto& e : exp) {
            DBOX got;
            if (!proto.receive(&got)) { ok = false; why = "receive() returned false"; break; }
            std::string pl(got.Payload.begin(), got.Payload.end());
            if (got.VPID != e.id || got.Stream != (uint8_t)e.st || pl != e.payload) {
                ok = false; why = "wrong packet: id=" + std::to_string(got.VPID) + " payload=\"" + pl + "\""; break;
            }
        }
    } catch (const SpinGuard& g) { ok = false; why = g.what(); }
      catch (const std::exception& x) { ok = false; why = std::string("crashed: ") + x.what(); }
    printf("%-4s %-42s checks=%-8ld waits=%-4ld most checks w/o rest=%ld%s%s\n",
        ok ? "PASS" : "FAIL", name, port.rawreads, port.waits, port.max_rawreads_without_wait,
        why.empty() ? "" : "  -> ", why.c_str());
    return ok;
}

// Run each test in a child process so a frozen receiver can be detected:
// if it hasn't finished after 5 seconds, it's stuck.
static bool run(const char* name, std::vector<uint8_t> bytes, std::vector<Expect> exp, bool trickle) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) { alarm(5); exit(run_inner(name, bytes, exp, trickle) ? 0 : 1); }
    int st = 0; waitpid(pid, &st, 0);
    if (WIFSIGNALED(st)) { printf("FAIL %-42s FROZE: still stuck after 5 seconds\n", name); return false; }
    return WEXITSTATUS(st) == 0;
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    int fails = 0;
    auto A = frame(3, 'M', "move 90");
    auto B = frame(4, 'S', "sensor ok");

    // 1. Normal packet, all bytes already here
    fails += !run("1 normal packet", A, {{3,'M',"move 90"}}, false);

    // 2. Bug 1: header still arriving over the wire
    fails += !run("2 [bug 1] header still arriving", A, {{3,'M',"move 90"}}, true);

    // 3. Bug 2: false 0x07 (from leftover payload) right before a real packet
    { std::vector<uint8_t> v = {0x07, 'x','y','z','w','q','r'}; add(v, A);
      fails += !run("3 [bug 2] false 0x07 then real packet", v, {{3,'M',"move 90"}}, false); }

    // 4. Bug 2: real packet whose header got corrupted, then a good one
    { auto bad = A; bad[1] ^= 0x10; std::vector<uint8_t> v = bad; add(v, B);
      fails += !run("4 [bug 2] corrupted header, then good one", v, {{4,'S',"sensor ok"}}, false); }

    // 5. Bug 2: false 0x07 whose 6 bytes overlap the real 0x07
    { std::vector<uint8_t> v = {0x07, 0x01, 0x02}; add(v, A);
      fails += !run("5 [bug 2] false 0x07 overlapping real one", v, {{3,'M',"move 90"}}, false); }

    // 6. Noise before a packet, payload containing 0x07, back-to-back packets
    { std::vector<uint8_t> v = {'b','o','o','t','\n'}; add(v, frame(1,'D',std::string("a\x07" "b",3))); add(v, B);
      fails += !run("6 noise + 0x07 inside payload + 2 packets", v, {{1,'D',std::string("a\x07" "b",3)},{4,'S',"sensor ok"}}, false); }

    // 7. Everything at once, arriving over time
    { std::vector<uint8_t> v = {0x07,'j','u','n','k','!','!'}; auto bad=A; bad[3]^=0x01; add(v,bad); add(v,B);
      fails += !run("7 garbage + bad header + good, trickling", v, {{4,'S',"sensor ok"}}, true); }

    printf("\n%d of 7 failed\n", fails);
    return fails;
}
