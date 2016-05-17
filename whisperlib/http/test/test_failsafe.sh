#!/bin/bash

bindir=$1
${bindir}/failsafe_test --servers=en.wikipedia.org,en.wikipedia.org --force_host_header=en.wikipedia.org --paths=/wiki/Main_Page,/wiki/Murder_of_Julia_Martha_Thomas,/wiki/Municipal_Borough_of_Richmond_\(Surrey\),/wiki/Local_Government_Act_1894,/wiki/London_Borough_of_Richmond_upon_Thames,/wiki/Edward_I_of_England,/wiki/Edward_II_of_England,/wiki/List_of_English_monarchs,/wiki/Egbert_of_Wessex,/wiki/Egbert_II_of_Kent,/wiki/William_the_Conqueror,/wiki/Louis_VIII_of_France,/wiki/Henry_III_of_England --alsologtostderr --cancel_every=3
