# Dockerfile for running Cortex build in Jenkins


**Build the docker image based on jenkins/jenkins**
~~~
docker build -t ie/jenkins .
~~~

**Run the jenkins server**
~~~
docker run -p 8080:8080 -v jenkins_home:/var/jenkins_home -p 50000:50000 ie/jenkins
~~~

**Data is stored in a Docker volume called jenkins_home**
~~~
docker volume inspect jenkins_home
~~~