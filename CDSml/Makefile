RTML=/home/jcmurphy/RTMLton/build/bin/mlton

build:
	$(RTML) -debug false -keep g -default-ann 'allowFFI true' runCD.mlb

dbgbuild:
	$(RTML) -debug true -keep g -default-ann 'allowFFI true' runCD.mlb

testrun:
	sudo ./runCD @MLton lower-bound 40 upper-bound 200 hpercent $(HPERC) rtthreads true max-heap $(HEAP) packingstage1 false packingstage2 false -- 

collect:
	sudo ./runCD @MLton lower-bound 40 upper-bound 200 hpercent $(HPERC) rtthreads true max-heap $(HEAP) packingstage1 false packingstage2 false -- 2> $(RUN)-pack0.txt
	sudo ./runCD @MLton lower-bound 40 upper-bound 200 hpercent $(HPERC) rtthreads true max-heap $(HEAP) packingstage1 true packingstage2 false -- 2> $(RUN)-pack1.txt
	sudo ./runCD @MLton lower-bound 40 upper-bound 200 hpercent $(HPERC) rtthreads true max-heap $(HEAP) packingstage1 true packingstage2 true -- 2> $(RUN)-pack2.txt

