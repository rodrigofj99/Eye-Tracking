from ctypes import sizeof
from random import shuffle
from sklearn.model_selection import train_test_split
from sklearn import svm
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np
from pathlib import Path
import random

positive = pd.read_csv(Path(__file__).parent /"../Outputs/scroll_down.csv")
negative = pd.read_csv(Path(__file__).parent /"../Outputs/no_scroll.csv")

mean = positive['x'].mean()
std = positive['x'].std()
print("The mean is:", mean,"; the standard deviation is:",std)
positive['x'] = positive['x'].apply(lambda x: (x-mean)/std)

new_index = [x for x in range(positive.shape[0], positive.shape[0] + negative.shape[0])]
negative = negative.set_axis(new_index,axis=0)
df = positive.append(negative)
y = []

for i in range(df['x'].size):
    if (i<=positive['x'].size):
        y.append(1)
    else: y.append(-1)
    
plt.hist(positive["x"])

# check which features are correlated and which one are correlated with the outcome variable
df_correlation = df.corr()
fig = plt.figure(figsize = (10,10))
sns.heatmap(df_correlation)

# Checking for outliers
df.describe()
sns.pairplot(df)
sns.boxplot(data=df)
plt.show()

X_train, X_test, y_train, y_test = train_test_split(df, y, test_size=0.33, random_state=42)#, shuffle=True)
svm = svm.SVC(kernel='linear')
svm.fit(X_train, y_train)
y_pred = svm.predict(X_test)
print("Accuracy:",svm.score(X_test, y_test))

w = svm.coef_
b = svm.intercept_
#a = np.array([0.628358,0.595343])
#new = np.dot(w,a)+b
#print(new)

print(w,b)
parameters = open(Path(__file__).parent /"../Outputs/parameters.txt","w")
w = np.array2string(w, precision=16, separator=' ').replace("[","").replace("]","")
b = np.array2string(b, precision=16).replace("[","").replace("]","")
L = w + "\n" + b
parameters.writelines(L)
parameters.close()

#y_a = svm.dual_coef_
#sv = svm.support_vectors_
#new2 = 0
#for i in range(len(sv)):
#    new2 += y_a[0][i]*np.dot(sv[i],a)+b