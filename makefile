.PHONY: up
up:
	docker-compose up -d
	docker exec -it os-linux-1 bash

.PHONY: stop
stop:
	docker-compose stop