CREATE EXTENSION IF NOT EXISTS "uuid-ossp" WITH SCHEMA public;
CREATE EXTENSION IF NOT EXISTS hstore WITH SCHEMA public;

CREATE TABLE cc_user (
id                UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
name              VARCHAR(255) NOT NULL,
created_at        TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now(),
updated_at        TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now()
);

CREATE TABLE question (
id                UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
body              TEXT NOT NULL,
created_at        TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now(),
updated_at        TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now()
);

CREATE TABLE vote (
user_id           UUID NOT NULL REFERENCES cc_user(id),
question_id       UUID NOT NULL REFERENCES question(id),
created_at        TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now(),
updated_at        TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now()
);
