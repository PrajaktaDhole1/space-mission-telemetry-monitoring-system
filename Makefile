all:
	gcc commander.c -o commander
	gcc satellite.c -o satellite -lpthread
	gcc telemetry.c -o receiver
	gcc orbit_analyzer.c -o analyzer
	gcc fault_recovery.c -o recovery
	gcc recorder.c -o recorder
	
run:
	./commander
	
clean:
	rm -f commander satellite receiver analyzer recovery reorder mission_log.txt
