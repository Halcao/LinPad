// Checks: an EPOLLONESHOT entry stays registered (disarmed) after it fires.
// Expected (Linux, epoll_ctl(2) "EPOLLONESHOT"): after the event is reported the fd stays
// in the interest list; EPOLL_CTL_MOD re-arms it and EPOLL_CTL_ADD fails with EEXIST.
// Observed upstream: the entry is removed when it fires (MOD/DEL give ENOENT, ADD succeeds
// again). The removal happens while the poll fd is still in use by the wait loop.
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
int main(void) {
    int p[2]; pipe(p); write(p[1], "x", 1);
    int ep = epoll_create1(0);
    struct epoll_event ev = { .events = EPOLLIN | EPOLLONESHOT, .data.fd = p[0] }, out;
    epoll_ctl(ep, EPOLL_CTL_ADD, p[0], &ev);
    int n1 = epoll_wait(ep, &out, 1, 100);
    int n2 = epoll_wait(ep, &out, 1, 100);
    printf("%s: first wait %d, second wait %d (want 1, 0)\n", n1 == 1 && n2 == 0 ? "PASS" : "FAIL", n1, n2);
    int m = epoll_ctl(ep, EPOLL_CTL_MOD, p[0], &ev), me = errno;
    printf("%s: MOD re-arms = %d errno %d (want 0)\n", m == 0 ? "PASS" : "FAIL", m, m ? me : 0);
    int n3 = epoll_wait(ep, &out, 1, 100);
    printf("%s: event after MOD %d (want 1)\n", n3 == 1 ? "PASS" : "FAIL", n3);
    int a = epoll_ctl(ep, EPOLL_CTL_ADD, p[0], &ev), ae = errno;
    printf("%s: ADD again = %d errno %d (want -1 EEXIST)\n", a == -1 && ae == EEXIST ? "PASS" : "FAIL", a, a ? ae : 0);
    return !(n1 == 1 && n2 == 0 && m == 0 && n3 == 1 && a == -1 && ae == EEXIST);
}
