// anyvalue-size-validation.slang

//DIAGNOSTIC_TEST:SIMPLE:-target cpp -stage compute -entry main -allow-dynamic-code

[anyValueSize(8)]
interface IInterface
{
    int doSomething();
};

struct S : IInterface
{
    uint a;
    uint b;
    uint c;
    int doSomething() { return 5; }
};

T test<T:IInterface>(T s)
{
    return s;
}

[numthreads(4, 1, 1)]
void main()
{
    S s;
    test(s);
}