#define YIELD \
  __SMACK_code("assume {:yield} true;")
    
#define CALLS_ASYNC \
  __SMACK_decl("var x: int;")

#define ASYNC_0_0(proc) \
  __SMACK_code("call {:async} @();", proc)

#define ASYNC_1_0(proc,arg) \
  __SMACK_code("call {:async} @(@);", proc, arg)

#define ASYNC_0_1(proc) \
  __SMACK_code("call {:async} x := @();", ret, proc)
    
#define BOOKMARK(b) \
  __SMACK_code("assume {:bookmark " #b "} true;")
    
#define ROUND(t,b,i,k) \
  __SMACK_code("assume {:round " #t ", " #b ", " #i ", " #k "} true;")
