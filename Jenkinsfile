#!groovy

parallel (


"nyu" : {node ('nyu')
                  {
                    deleteDir()
                    checkout scm
                    stage ('build_nyu')
                    {
                      sh "./build.sh $WORKSPACE $NODE_NAME $BRANCH_NAME"
                    }

                    stage ('run_nyu')
                    {
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 2 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 3 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 4 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 5 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./success.sh 2 nyu openfpm_vcluster"
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
                      sh "./build.sh $WORKSPACE $NODE_NAME $BRANCH_NAME"
                    }

                    stage ('run_sb15')
                    {
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 2 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 3 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 4 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 5 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 6 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./run.sh $WORKSPACE $NODE_NAME 7 $BRANCH_NAME"
                      sh "cd openfpm_vcluster && ./success.sh 2 sbalzarini-mac-15 openfpm_vcluster"
                    }
                  }
                 }

)

