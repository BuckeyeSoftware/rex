#ifndef RX_CORE_PP_H
#define RX_CORE_PP_H

#define RX_PP_PASTE(X, Y) X ## Y
#define RX_PP_STRINGIZE(X) #X
#define RX_PP_CAT(X, Y) RX_PP_PASTE(X, Y)

#endif // RX_CORE_PP_H
