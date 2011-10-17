
#pragma once

#define wait_for_threads( num_threads) { \
	if ( num_threads == 2) { \
		HANDLE events[2];\
		events[0]=threads[0].DoneEvent;\
		events[1]=threads[1].DoneEvent;\
		WaitForMultipleObjects(2, &events[0], true, INFINITE);\
	} else if ( num_threads == 3 ){ \
		HANDLE events[3];\
		events[0]=threads[0].DoneEvent;\
		events[1]=threads[1].DoneEvent;\
		events[2]=threads[2].DoneEvent;\
		WaitForMultipleObjects(3, &events[0], true, INFINITE);\
	} \
}
