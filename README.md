# ReasonConf 2019 Reason & GraphQL Workshop


[![CircleCI](https://circleci.com/gh/yourgithubhandle/workshop/tree/master.svg?style=svg)](https://circleci.com/gh/sgrove/graphql-workshop/tree/master)

`nodemon --delay "200ms" --watch executable/ --watch library/ --watch test/ -e re,ml,json,yml --exec "printf '\e]50;ClearScrollback\a' && esy x WorkshopApp.exe -p 9255"`

## Set up:    
                  
```               
npm install -g esy
git clone git@github.com:sgrove/graphql-workshop.git
cd graphql-workshop
esy install       
esy build         
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
