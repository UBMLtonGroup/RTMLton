FROM i386/ubuntu
RUN apt update -y
RUN apt upgrade -y
RUN apt install -y gcc libgmp-dev make smlnj patch mlton
CMD ["/bin/bash"]
