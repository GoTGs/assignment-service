FROM gcc AS development

WORKDIR /server/

RUN apt-get update

RUN apt-get install -y cmake libpq-dev libxslt-dev libxml2-dev libpam-dev libedit-dev libssl-dev nlohmann-json3-dev curl zip unzip tar

COPY dependencies .

COPY . .

FROM development AS builder

RUN cmake -S . -B build
RUN cmake --build build

RUN mkdir /app
RUN cp -r build/* /app

FROM builder AS production

EXPOSE 8003

CMD [ "stdbuf", "-oL", "./build/assignment" ]