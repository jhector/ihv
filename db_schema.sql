-- Remove old tables
DROP TABLE IF EXISTS `reason_malloc`;
DROP TABLE IF EXISTS `reason_free`;
DROP TABLE IF EXISTS `reason_realloc`;
DROP TABLE IF EXISTS `reason_calloc`;
DROP TABLE IF EXISTS `memory_write`;
DROP TABLE IF EXISTS `block`;
DROP TABLE IF EXISTS `snapshot`;

-- Create new
CREATE TABLE `snapshot` (
    `snapshot_id` INT(11) unsigned NOT NULL AUTO_INCREMENT,
    `time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `reason` INT(1) unsigned NOT NULL,  -- To know in which table we have to look in
    PRIMARY KEY (`snapshot_id`)
) ENGINE=InnoDB DEFAULT CHARSET=UTF8;

CREATE TABLE `block` (
    `block_id` INT(11) unsigned NOT NULL AUTO_INCREMENT,
    `alloc_snapshot_id` INT(11) unsigned NOT NULL,
    `free_snapshot_id` INT(11) unsigned NULL,
    `address` BIGINT unsigned NOT NULL,
    `size` BIGINT unsigned NOT NULL,
    PRIMARY KEY(`block_id`),
    KEY(`address`),
    CONSTRAINT `fk_alloc_block_snapshot`
        FOREIGN KEY (`alloc_snapshot_id`)
        REFERENCES snapshot (`snapshot_id`)
        ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=UTF8;

CREATE TABLE `memory_write` (
    `snapshot_id` INT(11) unsigned NOT NULL,
    `address` BIGINT unsigned NOT NULL,
    `value` CHAR(1) NOT NULL,
    PRIMARY KEY(`snapshot_id`, `address`),
    CONSTRAINT `fk_memory_write_snapshot`
        FOREIGN KEY (`snapshot_id`)
        REFERENCES snapshot (`snapshot_id`)
        ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=UTF8;

-- Snapshot generation reasons
CREATE TABLE `reason_malloc` (
    `snapshot_id` INT(11) unsigned NOT NULL,
    `address` BIGINT unsigned NOT NULL,
    `size` BIGINT unsigned NOT NULL,
    PRIMARY KEY (`snapshot_id`, `address`),
    KEY(`snapshot_id`),
    CONSTRAINT `fk_reason_malloc_snapshot`
        FOREIGN KEY (`snapshot_id`)
        REFERENCES snapshot (`snapshot_id`)
        ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=UTF8;

CREATE TABLE `reason_free` (
    `snapshot_id` INT(11) unsigned NOT NULL,
    `address` BIGINT unsigned NOT NULL,
    PRIMARY KEY (`snapshot_id`, `address`),
    KEY(`snapshot_id`),
    CONSTRAINT `fk_reason_free_snapshot`
        FOREIGN KEY (`snapshot_id`)
        REFERENCES snapshot (`snapshot_id`)
        ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=UTF8;

CREATE TABLE `reason_realloc` (
    `snapshot_id` INT(11) unsigned NOT NULL,
    `size` BIGINT unsigned NOT NULL,
    `old_address` BIGINT unsigned NOT NULL,
    `new_address` BIGINT unsigned NOT NULL,
    PRIMARY KEY (`snapshot_id`, `address`),
    KEY(`snapshot_id`),
    CONSTRAINT `fk_reason_realloc_snapshot`
        FOREIGN KEY (`snapshot_id`)
        REFERENCES snapshot (`snapshot_id`)
        ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=UTF8;

CREATE TABLE `reason_calloc` (
    `snapshot_id` INT(11) unsigned NOT NULL,
    `address` BIGINT unsigned NOT NULL,
    `member` BIGINT unsigned NOT NULL,
    `size` BIGINT unsigned NOT NULL,
    PRIMARY KEY (`snapshot_id`, `address`),
    KEY(`snapshot_id`),
    CONSTRAINT `fk_reason_calloc_snapshot`
        FOREIGN KEY (`snapshot_id`)
        REFERENCES snapshot (`snapshot_id`)
        ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=UTF8;

