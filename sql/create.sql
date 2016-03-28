CREATE TABLE last_update(id BIGSERIAL PRIMARY KEY, date timestamp, block_id integer not null);
ALTER TABLE last_update ADD CONSTRAINT constraint_last_update_block_id UNIQUE (block_id);

CREATE TABLE lines(id BIGSERIAL PRIMARY KEY, date timestamp not null, block_id integer not null, shape geometry not null);
CREATE INDEX idx_lines_shape ON lines USING GIST ( shape );

CREATE TABLE points(id BIGSERIAL PRIMARY KEY, date timestamp not null, block_id integer not null, shape geometry not null);
CREATE INDEX idx_points_shape ON points USING GIST ( shape );
