FROM jenkins/jenkins
MAINTAINER Don Boogert

USER root

RUN apt-get update 
RUN apt-get install -y scons
RUN apt-get install -y wget

USER jenkins