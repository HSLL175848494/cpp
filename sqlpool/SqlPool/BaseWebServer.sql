/*
 Navicat Premium Dump SQL

 Source Server         : ubuntu
 Source Server Type    : MySQL
 Source Server Version : 80039 (8.0.39-0ubuntu0.22.04.1)
 Source Host           : 192.168.6.131:3306
 Source Schema         : BaseWebServer

 Target Server Type    : MySQL
 Target Server Version : 80039 (8.0.39-0ubuntu0.22.04.1)
 File Encoding         : 65001

 Date: 08/11/2024 22:52:05
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user`  (
  `username` int NOT NULL AUTO_INCREMENT,
  `password` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  PRIMARY KEY (`username`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 2000002 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of user
-- ----------------------------
INSERT INTO `user` VALUES (2000000, '666666');
INSERT INTO `user` VALUES (2000001, '123456');

SET FOREIGN_KEY_CHECKS = 1;
