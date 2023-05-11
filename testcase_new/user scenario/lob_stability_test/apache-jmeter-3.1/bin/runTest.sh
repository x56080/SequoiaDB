set -e
for((i=0;i<100;i++)); do
	java -jar ../lib/ext/jmeter-lob-stab.jar --all
	./jmeter.sh -n -t ../../resource/exec-plan.jmx
	./jmeter.sh -n -t ../../resource/exec-plan.jmx
done
set +e
