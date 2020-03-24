1.	reset; make all


2.	For normal single manual testing

a.	./containers -t <# threads> -l <# loops/iterations> <target>

i.	Target = sglstack, sglqueue, treiber, ms, e_sgl, e_t, fc, basket


3.	For standard automatic testing

a.	./containers test


4.	For perf

a.	sudo perf stat -e page-faults ./containers … (rest of arguments)
