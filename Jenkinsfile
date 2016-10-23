#!groovy

parallel (


"nyu" : {node ('nyu')
                  {
                    deleteDir()
                    checkout scm
                    stage ('build_nyu')
                    {
                      sh "./build.sh $WORKSPACE $NODE_NAME"
                    }

                    stage ('run_nyu')
                    {
		      parallel(
                        "proc2" : {sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 2"}
                        "proc3" : {sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 3"}
                      )

                      sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 4"
                      sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 5"
                    }
                  }
                 },




"sb15" : {node ('sbalzarini-mac-15')
                  {
                    deleteDir()
                    env.PATH = "/usr/local/bin:${env.PATH}"
                    checkout scm
                    stage ('build_sb15')
                    {
                      sh "./build.sh $WORKSPACE $NODE_NAME NO"
                    }

                    stage ('run_sb15')
                    {
                      parallel(
                        "proc2" : {sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 2"}
                        "proc3" : {sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 3"}
                      )

                      sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 4"
                      sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 5"
                      sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 6"
                      sh "cd openfpm_data && ./run.sh $WORKSPACE $NODE_NAME 7"
                    }
                  }
                 }

)
