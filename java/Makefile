
JNAERATOR_VERSION=0.9.9
JNAERATOR_JAR=jnaerator-$(JNAERATOR_VERSION)-shaded.jar
JAVA=java
JAVAC=javac
JAR=jar

all: jar

jar: $(JNAERATOR_JAR) hidapi.jar

hidapi.jar: 
	$(JAVA) -jar $(JNAERATOR_JAR) -noMangling -library hidapi -noComp -o . ../hidapi/hidapi.h
	$(JAVAC) -cp $(JNAERATOR_JAR) hidapi/*.java
	$(JAR) cf hidapi.jar hidapi

$(JNAERATOR_JAR):
	wget -N http://nativelibs4java.sourceforge.net/maven/com/jnaerator/jnaerator/$(JNAERATOR_VERSION)/$(JNAERATOR_JAR)

clean:
	rm -f hidapi.jar
	rm -rf hidapi
