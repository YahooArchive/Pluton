JSRC=$(wildcard ../com/yahoo/pluton/*.java)
CLASSES=$(patsubst %.java,%.class,$(JSRC))
CLASSPATHS=$(patsubst ../%,%,$(CLASSES))

all libs:	pluton.jar

pluton.jar: $(CLASSES)
	echo Building jar file from: $(CLASSPATHS)
	cd .. && jar cf $@ $(CLASSPATHS)
	mv ../$@ .

$(CLASSES): $(JSRC)
	javac $(JSRC)

clean::
	rm -f pluton.jar $(CLASSES)
