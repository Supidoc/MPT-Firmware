ARG DOCKER_TAG=v6.1
FROM espressif/idf:${DOCKER_TAG}

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN apt-get update -y && apt-get install udev  -y



RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc


RUN /opt/esp/python_env/idf6.1_py3.12_env/bin/pip install --no-cache-dir pre-commit astyle_py==1.0.5

ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]

CMD ["/bin/bash", "-c"]
