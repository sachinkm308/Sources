LIB "tst.lib";
tst_init();
LIB "ncfactor.lib";

ring R = 0,(x,d),dp;
def r = nc_algebra(1,1);
setring(r);
poly L = (1+x^2*d)^4;
facFirstWeyl(L);

tst_status();
tst_status(1); $
