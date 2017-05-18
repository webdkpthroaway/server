INSERT INTO `migrations` VALUES ('20170518200504'); 

# Add scaling to Primal Blessing (Thekal's/Arlokk's Grasp set bonus proc)
UPDATE `creature_template` SET `scale`='2.0' WHERE `entry`='15109';