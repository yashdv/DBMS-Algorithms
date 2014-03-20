Basic MYSQL Parser and Engine
=============================


**********COMPILING************************************************************

make

**********EXECUTING***********************************************************

./megatron201001015 <schema file name>

**********EXAMPLE QUERIES*****************************************************

1.  
select * from countries; | o1  
select * from airports; | o1  

2.  
select NAME           from countries;           | o1  
select NAME,CONTINENT from countries;           | o1  
select NAME,LATITUDE  from airports;            | o1  
select TYPE, CODE     from countries, airports; | o1  

3.  
select distinct(CODE)      from countries; | o1  
select distinct(CONTINENT) from countries; | o1  

4.  
select *                   from airports where TYPE = "heliport" ;    | o1  
select *                   from airports where TYPE = 'heliport' ;    | o1  
select ID, COUNTRY         from airports where COUNTRY = "US";        | o1  
select LATITUDE, LONGITUDE from airports where LONGITUDE <= 66;       | o1  
select LATITUDE, LONGITUDE from airports where LATITUDE <= 10;        | o1  
select LATITUDE, LONGITUDE from airports where LATITUDE <= -30;       | o1  
select LATITUDE, LONGITUDE from airports where LATITUDE <= LONGITUDE; | o1  
select *                   from airports where LATITUDE = LONGITUDE;  | o1  

5.  
select *            from countries, airports where CODE = COUNTRY; | o1  
select CODE, COUNTRY from countries, airports where CODE = COUNTRY; | o1  
