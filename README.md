# Run Quartus with GUI from docker

In one shell:
```shell
xhost local:root
docker run -it --rm --net=host --name ias -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix:rw  -v `pwd`:/src:ro ias:1.2.1
```

In another:
```shell
docker cp /etc/machine-id ias:/etc/machine-id
```

Then go back to the shell with the active container:
```shell
quartus
```
