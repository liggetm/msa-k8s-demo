# msa-k8s-demo

My attempt at a full application lifecycle demo for Kubernetes - hopefully highlighting features available in Kubernetes for application developers.

## Pre-reqs

- `kubectl` installed and configured with an appropriate token for working with Kubernetes
- `gcc` if you want to compile the local app
- `docker`, ideally natively if you want to build/run the app image (to push to the in-cluster registry you need to configure it as an insecure-registry)

### Local app (optional)

Statically compile the demo c-app:
```
$ gcc server_v1.c -o server_v1
```
Run the c-app (where en3 is the interface name the webservice should run and display the IP of):
```
$ ./server_v1 en3
```
Verify in a browser using: http://localhost:8080 - you should see the IP address of en3.

### Cross-compile the app for Alpine Linux

Cross-compile the c-app for alpine Linux into the docker folder:
```
$ docker run --rm -v `pwd`:/tmp frolvlad/alpine-gcc gcc --static /tmp/c-app/server_v1.c -o /tmp/docker/server
```

### Build a docker image with the app baked in

Build the docker image:
```
$ docker build -t c-app docker
```

Confirm image is available:
```
$ docker images

REPOSITORY            TAG                 IMAGE ID            CREATED             SIZE
c-app                 latest              41e9fd52db08        10 seconds ago      4.19 MB
frolvlad/alpine-gcc   latest              f487830374fa        4 weeks ago         101 MB
alpine                latest              4a415e366388        4 weeks ago         3.99 MB
```

Optionally run the docker image locally:
```
$ docker run -p8080:8080 -d --name c-app c-app
```

Verify in a browser using: http://localhost:8080 - you should see the IP address of the docker host itself.

### Push the new docker image to the in-cluster registry

Deploy the image to the in-cluster docker registry:
```
$ kubectl create -f kubernetes/registry/cluster-registry.yml
```
Tag the c-app v1 docker image for the remote registry (in this case on atomic80 port 30005):
```
docker tag b2614d4c20cc atomic80:30005/c-app:v1
```
Push the docker image to the remote registry:
```
$ docker push atomic80:30005/c-app:v1
```
View the images on the remote registry:
```
$ docker images atomic80:30005/*
```

_Now repeat for the version 2 copy of the c-app, changed source file and iterated version info._
```
$ docker run --rm -v `pwd`:/tmp frolvlad/alpine-gcc gcc --static /tmp/c-app/server_v2.c -o /tmp/docker/server
$ docker build -t c-app docker
$ docker push atomic80:30005/c-app:v2
```

### Run the app in the cluster

Deploy the kubernetes c-app v1 in a pod:
```
$ kubectl create -f kubernetes/c-app/c-app-pod.yml
```
Response:
```
$ kubectl get pods
NAME               READY     STATUS    RESTARTS   AGE
c-app              1/1       Running   0          22s
cluster-registry   1/1       Running   0          14m
```
Deploy a kubernetes service to connect to the app.
```
$ kubectl create -f kubernetes/c-app/c-app-service.yml
```
On the local system use kubectl proxy to connect to the service:
```
$ kubectl proxy --port=8080 &
```
Now try to browse to the service via the proxy on http://localhost:8080/api/v1/proxy/namespaces/default/services/c-app/

Cleanup:
```
$ kubectl delete -f kubernetes/c-app/c-app-pod.yml
$ kubectl create -f kubernetes/c-app/c-app-service.yml
```

### Pod Probes

Deploy the app with probes enabled:
```
$ kubectl create -f kubernetes/c-app/c-app-pod-probes.yml
```
Expose the probes using a service:
```
$ kubectl delete -f kubernetes/c-app/c-app-service-probes.yml
```
Now browse via the proxy to http://localhost:8080/api/v1/proxy/namespaces/default/services/c-app:9999/healthz - the original service is still running but now on http://localhost:8080/api/v1/proxy/namespaces/default/services/c-app:9998

Cleanup:
```
$ kubectl delete -f kubernetes/c-app/c-app-pod-probes.yml
$ kubectl create -f kubernetes/c-app/c-app-service-probes.yml
```

### Pod environment variables and secrets

Create a cluster secret:
```
$ kubectl create secret generic secret.cookie --from-file=kubernetes/c-app/secret.cookie
```
Create a config-map:
```
$ kubectl create -f kubernetes/c-app/c-app-config-map.yml
```
Deploy a pod with variables and secret access:
```
$ kubectl create -f kubernetes/c-app/c-app-pod-env.yml
```
Cleanup:
```
$ kubectl delete secret secret.cookie
$ kubectl delete -f kubernetes/c-app/c-app-config-map.yml
$ kubectl delete -f kubernetes/c-app/c-app-pod-env.yml
```

### Jobs

Deploy our c-app sleep job:
```
$ kubectl create -f kubernetes/c-app/c-app-job.yml
```
View the job status:
```
$ kubectl get jobs
```
Cleanup:
```
$ kubectl delete -f kubernetes/c-app/c-app-job.yml
```

### Replication controllers

Re-enable the non-probe service:
```
$ kubectl create -f kubernetes/c-app/c-app-service.yml
```
Deploy the replication controller:
```
$ kubectl create -f kubernetes/c-app/c-app-rc.yml
```

Repeatedly browsing to http://localhost:8080/api/v1/proxy/namespaces/default/services/c-app/ should show different IP addresses.

Scale up the number of replicas:
```
$ kubectl scale --replicas=8 rc/c-app
```
Configure the replication controller for auto-scaling:
```
kubectl autoscale rc c-app --min=2 --max=8 --cpu-percent=20
```
_Note: this cpu-percent is the target running usage per pod._

Cleanup:
```
$ kubectl delete -f kubernetes/c-app/c-app-rc.yml
```

### Deployments

Deploy the c-app deployment version 1:
```
$ kubectl create -f kubernetes/c-app/c-app-deployment.yml --record
```
Show the deployments
```
$ kubectl get deployments
```

Now upgrade to version 2:
```
$ kubectl apply -f kubernetes/c-app/c-app-deployment-v2.yml --record
```
Look at the rollout history:
```
$ kubectl rollout history deployment c-app
```
Scale the deployment:
```
$ kubectl scale --replicas=10 deployments/c-app
```
Rollback the last rollout:
```
$ kubectl rollout undo deployment c-app --record
```
Cleanup:
```
$ kubectl delete -f kubernetes/c-app/c-app-deployment-v2.yml
```

### Storage

Create a persistent volume:
```
$ kubectl create -f kubernetes/storage/pv.yml
```
Create a stateful set:
```
$ kubectl create -f kubernetes/storage/c-app-stateful-set.yml
```
View the predictable hostname:
```
$ kubectl exec -it c-app-1 -- hostname
```
Persist a file on c-app-1:
```
$ kubectl exec -it c-app-1 -- touch /data/file

```
You should now be able to see that file on the real filesystem of the node.
Delete pod c-app-1:
```
$ kubectl delete pod c-app-1
```
It will restart on the same name and you should be able to still see the file:
```
$ kubectl exec -it c-app-1 -- ls /data
```

Cleanup:
```
$ kubectl delete -f kubernetes/storage/c-app-stateful-set.yml
$ kubectl create -f kubernetes/storage/pv.yml
$ kubectl delete pvc my-data-c-app-0
$ kubectl delete pvc my-data-c-app-1
```
To fully cleanup, you need to delete the /var/data/vol* directories.


### External access (not via kubectl proxy)

Already doing nodePort access for the in-cluster registry:
```
$ kubectl describe svc cluster-registry
```
Configure our ingress backend:
```
$ kubectl create -f kubernetes/ingress-udp/ingress-backend-pod.yml
$ kubectl create -f kubernetes/ingress-udp/ingress-backend-svc.yml
```
Configure our settings config-map for the ingress controller:
```
$ kubectl create -f kubernetes/ingress-udp/ingress-dns-config-map.yml
```
Now start the ingress controller itself:
```
$ kubectl create -f kubernetes/ingress-udp/ingress-controller-pod.yml
```
Test by running a nslookup for "kubernetes.default.svc.cluster.local" against the host with that the pod is scheduled on, eg:
```
$ nslookup kubernetes.default.svc.cluster.local 172.26.136.80
```
Remove controller pod and create a daemonset:
```
$ kubectl delete -f kubernetes/ingress-udp/ingress-controller-pod.yml
$ kubectl create -f kubernetes/ingress-udp/ingress-controller-ds.yml
```
Cleanup:
```
$ kubectl delete -f kubernetes/ingress-udp/ingress-controller-ds.yml
$ kubectl delete -f kubernetes/ingress-udp/ingress-backend-pod.yml
$ kubectl delete -f kubernetes/ingress-udp/ingress-backend-svc.yml
$ kubectl delete -f kubernetes/ingress-udp/ingress-dns-config-map.yml
```
