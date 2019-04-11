# ReasonConf 2019 Reason & GraphQL Workshop


[![CircleCI](https://circleci.com/gh/sgrove/graphql-workshop/tree/master.svg?style=svg)](https://circleci.com/gh/sgrove/graphql-workshop/tree/master)

`nodemon --delay "200ms" --watch executable/ --watch library/ --watch test/ -e re,ml,json,yml --exec "printf '\e]50;ClearScrollback\a' && esy x WorkshopApp.exe -p 9255"`

## Set up:    
                  
```               
npm install -g esy
git clone git@github.com:sgrove/graphql-workshop.git
cd graphql-workshop
esy install       
esy pesy
esy dune build -p workshop
esy dune exec executable/WorkshopApp.exe
```               

## Running Binary:

After building the project, you can run the main binary that is produced.

```
esy install && esy pesy && esy dune build -p workshop && esy dune exec executable/WorkshopApp.exe 
```

## Server Architecture
+-----------------+           +------------+
| HttpServer.Util |           |TopResolvers|
+-------|---------+           +-----|------+
        |                           |
  +-----|----+                +-----|-------+
  |HttpServer|                |GraphQLServer|
  +-----|----+                +-----|-------+
        |                           |
        |                      +----|----+
        +----------------------+WebRoutes|
        |                      +----|----+
        |                           |    
        |        +---------+        | 
        +--------|TCPServer|--------+             
                 +----|----+
                      |
                +-----|-----+
                |WorkshopApp|
                +-----------+

## Updating the local GraphQL schema
`update_schema.sh http://localhost:8080/v1alpha1/graphql`

## Dev snippets

To experiment with Hasura migrations and then reset, this snippet is handy:
`dropdb cc_local && createdb cc_local && ./bin/run_migrations.sh 'postgres://localhost:5432/cc_local?user=postgres&password=&sslmode=disable'`
