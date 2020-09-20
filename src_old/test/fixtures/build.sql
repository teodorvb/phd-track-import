-- Build Tables
create table sequence_set (
  id              bigserial,
  created_at      timestamp not null,
  info            text not null,
  frame_dim       int not null,
  frame_info  	  text,
  primary key(id)
);

create table sequence (
  id              bigserial,
  sequence_set_id bigint not null,

  source_data_set varchar(2048),
  source_track_id int,
  
  category       smallint,

  track_length	 int not null,
  frame_dim	 int not null,
  data           bytea not null,
  data_size	 int not null,
  data_type_size int not null,
  source_range_low int,
  source_range_high int,

  primary key(id),
  foreign key(sequence_set_id) references sequence_set on delete cascade
);

-- Insert Data
insert into sequence_set
(id, info, frame_dim, created_at) values((select nextval('sequence_set_id_seq')), 'Dummy', 2, now());



