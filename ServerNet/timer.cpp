//������Linuxƽ̨�Ķ�ʱ����
#ifndef _WIN32
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#define TIME_WHEEL_SIZE 8
typedef void (*func)(int data);
struct timer_node {
    struct timer_node* next;
    int rotation;
    func proc;
    int data;
};
struct timer_wheel {
    struct timer_node* slot[TIME_WHEEL_SIZE];
    int current;
};
struct timer_wheel timer = { {0}, 0 };
void tick(int signo)
{
    // ʹ�ö���ָ��ɾ���е������ɾ��
    struct timer_node** cur = &timer.slot[timer.current];
    while (*cur) {
        struct timer_node* curr = *cur;
        if (curr->rotation > 0) {
            curr->rotation--;
            cur = &curr->next;
        }
        else {
            curr->proc(curr->data);
            *cur = curr->next;
            free(curr);
        }
    }
    timer.current = (timer.current + 1) % TIME_WHEEL_SIZE;
    alarm(1);
}
void add_timer(int len, func action)
{
    int pos = (len + timer.current) % TIME_WHEEL_SIZE;
    struct timer_node* node = malloc(sizeof(struct timer_node));
    // ���뵽��Ӧ���ӵ�����ͷ������, O(1)���Ӷ�
    node->next = timer.slot[pos];
    timer.slot[pos] = node;
    node->rotation = len / TIME_WHEEL_SIZE;
    node->data = 0;
    node->proc = action;
}
// test case1: 1sѭ����ʱ��
int g_sec = 0;
void do_time1(int data)
{
    printf("timer %s, %d\n", __FUNCTION__, g_sec++);
    add_timer(1, do_time1);
}
// test case2: 2s���ζ�ʱ��
void do_time2(int data)
{
    printf("timer %s\n", __FUNCTION__);
}
// test case3: 9sѭ����ʱ��
void do_time9(int data)
{
    printf("timer %s\n", __FUNCTION__);
    add_timer(9, do_time9);
}
int main()
{
    signal(SIGALRM, tick);
    alarm(1); // 1s����������
    // test
    add_timer(1, do_time1);
    add_timer(2, do_time2);
    add_timer(9, do_time9);
    while (1) pause();
    return 0;
}
#endif // !_WIN32

