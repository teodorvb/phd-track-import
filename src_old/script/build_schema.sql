create table if not exists sequence_set (
  id              bigserial,
  created_at      timestamp not null,
  info            text not null,
  frame_dim       int not null,
  frame_info  	  text,
  primary key(id)
);

create table if not exists sequence (
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



create table if not exists experiment (
  id   	     	bigserial,
  info		text not null,
  finger_print  varchar(256),
  created_at	timestamp not null,
  primary key   (id)  
);


create table if not exists experiment_property (
  id  	bigserial,
  k	varchar(1024),
  val	varchar(1024),
  experiment_id bigint,

  primary key(id),
  foreign key(experiment_id) references experiment on delete cascade
);


create table if not exists experiment_result (
  id   	bigserial,
  k	varchar(1024),
  val	numeric,
  sample_id bigint,
  experiment_id bigint,

  primary key(id),
  foreign key(experiment_id) references experiment on delete cascade
);

create table if not exists experiment_plot (
  id   	bigserial,
  k	varchar(1024),
  val	varchar(256),
  sample_id bigint,
  experiment_id bigint,

  primary key(id),
  foreign key(experiment_id) references experiment on delete cascade
);


create table if not exists experiment_log (
  id   	bigserial,
  k	varchar(1024),
  val	bytea,
  sample_id bigint,
  data_size	 int,
  data_type_size int,

  experiment_id bigint not null,


  primary key(id),
  foreign key(experiment_id) references experiment on delete cascade
);

create table if not exists experiment_sequence_set (
  id   	     	  bigserial,
  k    	     	  text not null,
  experiment_id   bigint not null,
  sequence_set_id bigint not null,

  primary key (id),
  foreign key (experiment_id) references experiment on delete cascade,
  foreign key (sequence_set_id) references sequence_set on delete restrict,
  unique(k, experiment_id, sequence_set_id)

);


