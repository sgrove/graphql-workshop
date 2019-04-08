# workshop


[![CircleCI](https://circleci.com/gh/yourgithubhandle/workshop/tree/master.svg?style=svg)](https://circleci.com/gh/yourgithubhandle/workshop/tree/master)

`nodemon --delay "200ms" --watch executable/ --watch library/ --watch test/ -e re,ml,json,yml --exec "printf '\e]50;ClearScrollback\a' && esy x WorkshopApp.exe -p 9255"`


**Contains the following libraries and executables:**

```
workshop@0.0.0
│
├─test/
│   name:    TestWorkshop.exe
│   main:    TestWorkshop
│   require: workshop.lib
│
├─library/
│   library name: workshop.lib
│   namespace:    Workshop
│   require:
│
└─executable/
    name:    WorkshopApp.exe
    main:    WorkshopApp
    require: workshop.lib
```

## Developing:

```
npm install -g esy
git clone <this-repo>
esy install
esy build
```

## Running Binary:

After building the project, you can run the main binary that is produced.

```
esy x WorkshopApp.exe 
```

## Running Tests:

```
# Runs the "test" command in `package.json`.
esy test
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
