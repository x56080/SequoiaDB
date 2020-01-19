========================
 S3 compatibility tests
========================

This is a set of unofficial Amazon AWS S3 compatibility
tests, that can be useful to people implementing software
that exposes an S3-like API. The tests use the  Boto3 libraries.

The tests use the unittest test framework. To get started, ensure you have
the ``virtualenv`` software installed; e.g. on Debian/Ubuntu::

	sudo apt-get install python-virtualenv

and then run::

    ./bootstrap

You will need to create a configuration file with the location of the
service and two different credentials. A sample configuration file named
``s3tests.json`` has been provided in this repo. This file can be
used to run the s3 tests on a sequoias3 cluster started with vstart.

Once you have that file copied and edited, you can run the tests with::

    ./virtualenv/bin/python -m unittest

You can specify which directory of tests to run::

	./virtualenv/bin/python -m unittest s3tests_boto3.functional

You can specify which file of tests to run::

	./virtualenv/bin/python -m unittest s3tests_boto3.functional.test_bucket_object_support

You can specify which test to run::

	./virtualenv/bin/python -m unittest s3tests_boto3.functional.test_bucket_object_support.TestBucketObject.test_bucket_list_empty

You can specify which pattern tests to run::

	 ./virtualenv/bin/python -m unittest discover -p "test*.py"

Some tests have attributes set based on their current reliability and
things like AWS not enforcing their spec stricly. You can filter tests
based on their attributes::

	./virtualenv/bin/python -a '!fails_on_aws'

Most of the tests have both Boto3 and Boto2 versions. Tests written in
Boto2 are in the ``s3tests`` directory. Tests written in Boto3 are
located in the ``s3test_boto3`` directory.

You can run only the boto3 tests with::

    ./virtualenv/bin/nosetests -v -s -A 'not fails_on_rgw' s3tests_boto3.functional

