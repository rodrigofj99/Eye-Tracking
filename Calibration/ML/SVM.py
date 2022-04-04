from ctypes import sizeof
from random import shuffle
from sklearn.model_selection import train_test_split
from sklearn.feature_selection import SequentialFeatureSelector
from sklearn.feature_selection import RFECV
from sklearn import svm
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np
from pathlib import Path
import random


#Reading files from calibration
scroll_down = pd.read_csv(Path(__file__).parent /"../Outputs/scroll_down.csv")
scroll_up = pd.read_csv(Path(__file__).parent /"../Outputs/scroll_up.csv")
no_scroll = pd.read_csv(Path(__file__).parent /"../Outputs/no_scroll.csv")
no_zoom = pd.read_csv(Path(__file__).parent /"../Outputs/no_zoom.csv")
zoom_in = pd.read_csv(Path(__file__).parent /"../Outputs/zoom_in.csv")

#mean = positive['x'].mean()
#std = positive['x'].std()
#print("The mean is:", mean,"; the standard deviation is:",std)
#positive['x'] = positive['x'].apply(lambda x: (x-mean)/std)


#### Initialization and analysis of dataframes ####
index_scroll_down = [x for x in range(scroll_down.shape[0], scroll_down.shape[0] + no_scroll.shape[0])]
no_scroll = no_scroll.set_axis(index_scroll_down,axis=0)
df_down = scroll_down.append(no_scroll)

index_scroll_up = [x for x in range(scroll_up.shape[0], scroll_up.shape[0] + no_scroll.shape[0])]
no_scroll = no_scroll.set_axis(index_scroll_up,axis=0)
df_up = scroll_up.append(no_scroll)

index_zoom = [x for x in range(zoom_in.shape[0], zoom_in.shape[0] + no_zoom.shape[0])]
no_zoom = no_zoom.set_axis(index_zoom,axis=0)
df_zoom = zoom_in.append(no_zoom)

y_down = []
y_up = []
y_zoom = []

f,ax = plt.subplots(2,4,figsize = (10,10))

for i in range(df_down['x'].size):
    if (i<=scroll_down['x'].size):
        y_down.append(1)
    else: y_down.append(-1)
    
#ax[0,0].hist(scroll_down["x"])

for i in range(df_up['x'].size):
    if (i<=scroll_up['x'].size):
        y_up.append(1)
    else: y_up.append(-1)
    
#ax[1,0].hist(scroll_up["x"])

for i in range(df_zoom[" Pos z"].size):
    if (i<=zoom_in["Pos x"].size):
        y_zoom.append(1)
    else: y_zoom.append(-1)
    
#ax[0,1].hist(zoom_in[" Pos z"])


# check which features are correlated and which one are correlated with the outcome variable
#df_down_correlation = df_down.corr()
#sns.heatmap(df_down_correlation, ax=ax[1,1])

# Checking for outliers
#df_down.describe()
#sns.pairplot(df_down)
#sns.boxplot(data=df_down)

#df_up_correlation = df_up.corr()
#sns.heatmap(df_up_correlation, ax=ax[0,2])

#df_up.describe()
#sns.pairplot(df_up)
#sns.boxplot(data=df_up)

#df_zoom_correlation = df_zoom.corr()
#sns.heatmap(df_zoom_correlation, annot=True, ax=ax[1,2])

#df_zoom.describe()
#sns.pairplot(df_zoom)
#sns.boxplot(data=df_zoom)


#### Training of ML models ####
X_train, X_test, y_train, y_test = train_test_split(df_down, y_down, test_size=0.33, random_state=42)#, shuffle=True)
svm_down = svm.SVC(kernel='linear')
svm_down.fit(X_train, y_train)
y_pred = svm_down.predict(X_test)
print("Accuracy for scroll down:",svm_down.score(X_test, y_test))

X_train, X_test, y_train, y_test = train_test_split(df_up, y_up, test_size=0.33, random_state=42)#, shuffle=True)
svm_up = svm.SVC(kernel='linear')
svm_up.fit(X_train, y_train)
y_pred = svm_up.predict(X_test)
print("Accuracy for scroll up:",svm_up.score(X_test, y_test))

X_train, X_test, y_train, y_test = train_test_split(df_zoom, y_zoom, test_size=0.33, random_state=42)#, shuffle=True)
svm_zoom = svm.SVC(kernel='linear')
svm_zoom.fit(X_train, y_train)
y_pred = svm_zoom.predict(X_test)
print("Accuracy for zooming with all features:",svm_zoom.score(X_test, y_test))


importance = np.abs(svm_zoom.coef_).flatten()
feature_names = np.array(df_zoom.columns)

#plt.bar(height=importance, x=feature_names)
ax[0,3].bar(height=importance, x=feature_names)
#plt.title("Feature importances via coefficients")
#ax[1].title("Feature importances via coefficients")


#sfs_forward = SequentialFeatureSelector(
#    svm_zoom, n_features_to_select=2, direction="forward", cv=5, n_jobs=-1 #5-fold cross-validation and running in parallel
#).fit(df_zoom, y_zoom)

#sfs_backward = SequentialFeatureSelector(
#    svm_zoom, n_features_to_select=2, direction="backward", cv=5, n_jobs=-1
#).fit(X_train, y_train)

#rfecv = RFECV(svm_zoom, min_features_to_select=2, cv=5, n_jobs=-1).fit(X_train, y_train)

#print(
#    "Features selected by forward sequential selection: "
#    f"{sfs_forward.get_feature_names_out()}"
#)
#print(
#    "Features selected by backward sequential selection: "
#    f"{sfs_backward.get_feature_names_out()}"
#)

#print(
#    "Features selected by recursive feature elimination: "
#    f"{rfecv.get_feature_names_out()}"
#)

#plt.show()

X_train, X_test, y_train, y_test = train_test_split(df_zoom[[' Pos z']], y_zoom, test_size=0.33, random_state=42)#, shuffle=True)
svm_zoom = svm.SVC(kernel='linear')
svm_zoom.fit(X_train, y_train)
y_pred = svm_zoom.predict(X_test)
print("Accuracy for zooming with one feature:",svm_zoom.score(X_test, y_test))

X_train, X_test, y_train, y_test = train_test_split(df_zoom[[' Pos z', 'Pos x']], y_zoom, test_size=0.33, random_state=42)#, shuffle=True)
svm_zoom = svm.SVC(kernel='linear')
svm_zoom.fit(X_train, y_train)
y_pred = svm_zoom.predict(X_test)
print("Accuracy for zooming with two features:",svm_zoom.score(X_test, y_test))

X_train, X_test, y_train, y_test = train_test_split(df_zoom[[' Pos z', 'Pos x', ' Rot z ']], y_zoom, test_size=0.33, random_state=42)#, shuffle=True)
svm_zoom = svm.SVC(kernel='linear')
svm_zoom.fit(X_train, y_train)
y_pred = svm_zoom.predict(X_test)
print("Accuracy for zooming with three features:",svm_zoom.score(X_test, y_test))



#Best zooming training
X_train, X_test, y_train, y_test = train_test_split(df_zoom[[' Pos z']], y_zoom, test_size=0.33, random_state=42)#, shuffle=True)
svm_zoom = svm.SVC(kernel='linear')
svm_zoom.fit(X_train, y_train)


#### Output file with coefficients ####
parameters = open(Path(__file__).parent /"../Outputs/parameters.txt","w")
for i in range(3):
    if (i == 0):
        w = svm_down.coef_
        b = svm_down.intercept_
        print("Scroll down: w =", w, "b =", b)
    elif (i == 1):
        w = svm_up.coef_
        b = svm_up.intercept_
        print("Scroll up: w =", w, "b =", b)
    else:
        w = svm_zoom.coef_
        b = svm_zoom.intercept_
        print("Zoom: w =", w, "b =", b)
    
    w = np.array2string(w, precision=16, separator=' ').replace("[","").replace("]","")
    b = np.array2string(b, precision=16).replace("[","").replace("]","")
    L = w + "\n" + b + "\n"
    parameters.writelines(L)
parameters.close()
