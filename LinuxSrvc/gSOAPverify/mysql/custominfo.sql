-- MySQL dump 10.13  Distrib 5.6.35, for Linux (i686)
--
-- Host: localhost    Database: custominfo
-- ------------------------------------------------------
-- Server version	5.6.35

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

USE custominfo;

--
-- Table structure for table `glkline`
--

DROP TABLE IF EXISTS `glkline`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `glkline` (
  `idx` int(11) unsigned zerofill NOT NULL AUTO_INCREMENT,
  `user` varchar(24) NOT NULL,
  `psw` varchar(64) NOT NULL,
  `img` varchar(32) DEFAULT NULL,
  `sex` char(4) DEFAULT NULL,
  `age` int(3) DEFAULT NULL,
  `zip` int(8) DEFAULT NULL,
  `tell` varchar(14) DEFAULT NULL,
  `date` date DEFAULT NULL,
  `email` varchar(32) DEFAULT NULL,
  `site` varchar(64) DEFAULT NULL,
  `desc` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`idx`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `glkline`
--

LOCK TABLES `glkline` WRITE;
/*!40000 ALTER TABLE `glkline` DISABLE KEYS */;
INSERT INTO `glkline` VALUES (0,'ioscatchme','a6afbbcbf8be7668',NULL,'m',22,NULL,'13700000001','2017-12-07','shenyun_n@foxmail.com','www.shenyun.link',NULL),(1,'ccccc','88888',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
/*!40000 ALTER TABLE `glkline` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2018-01-14 18:49:15
