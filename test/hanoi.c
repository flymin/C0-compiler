void hanoi(int n, char from, char tmp, char to)
{
    if (n > 0) {
        hanoi(n - 1, from, to, tmp);
        printf("take ",n);
        printf("from ", from);
        printf("to ",to);
        hanoi(n - 1, tmp, from, to);
    }
    return;
}

void main()
{
    hanoi(4,'a','b','c');
    return;
}