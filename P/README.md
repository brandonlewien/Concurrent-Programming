reset; make all

---
### For normal single manual testing

./containers -t <# threads> -l <# loops/iterations> <target>

###### Target = sglstack, sglqueue, treiber, ms, e_sgl, e_t, fc, basket
---

### For standard automatic testing

./containers test

---
### For perf

perf stat -e page-faults ./containers … (rest of arguments)
