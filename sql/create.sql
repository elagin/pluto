CREATE TABLE last_update(id BIGSERIAL PRIMARY KEY, date timestamp, block_id integer not null);
CREATE TABLE lines(id BIGSERIAL PRIMARY KEY, date timestamp not null, block_id integer not null, shape geometry);
CREATE TABLE points(id BIGSERIAL PRIMARY KEY, date timestamp not null, block_id integer not null, shape geometry);


