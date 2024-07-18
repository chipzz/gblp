#define minmax(x,lo,hi)	( (x)<(lo)?(lo):( (x)>(hi)?(hi):(x)) )
#define putminmax(x,lo,hi) x = minmax(x,lo,hi)