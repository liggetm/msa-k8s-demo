# msa-k8s-demo

Statically compile the c-app:
```
$ gcc server.c -o server
```
Run the c-app:
```
$ ./server en3
```
(where en3 is the interface name)

Cross-compile the c-app for alpine into the docker folder:
```
$ cd src
$ docker run --rm -v `pwd`:/tmp frolvlad/alpine-gcc gcc --static /tmp/c-app/server.c -o /tmp/docker/server
```

Build the docker image:
```
$ cd docker
$ docker build -t c-app .
```

Confirm image is available:
```
$ docker images

REPOSITORY                                       TAG                 IMAGE ID            CREATED             SIZE
c-app-image                                      latest              36e55b910102        6 seconds ago       4.19 MB
```

Run the docker app:
```
$ docker run -p8080:8080 -d --name c-app c-app-image
```

Verify in a browser using: http://localhost:8080

You should see the IP address of the docker host.
