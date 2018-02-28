import theano
import theano.tensor as T
import pdb
import numpy as np

class LogisticRegression:
    
    def __init__(self, dimension, num_classes):
        X = T.matrix('X')
        y = T.ivector('y')
        learning_rate = T.scalar('learning_rate')
        
        # initialize with 0 the weights W as a matrix
        self.W = theano.shared(value=np.zeros((dimension, num_classes),
                                              dtype=theano.config.floatX),
                               name='W',
                               borrow=True)

        # initialize the biases b as a vector of 0s
        self.b = theano.shared(value=np.zeros((num_classes,),
                                              dtype=theano.config.floatX),
                               name='b',
                               borrow=True)

        self.batch_size = 600
        
        self.params = [self.W, self.b]
        pred_cls_given_x = T.nnet.softmax(T.dot(X, self.W) + self.b)
        pred_ids = T.argmax(pred_cls_given_x, axis=1)
        cost =  -T.mean(T.log(pred_cls_given_x)[T.arange(y.shape[0]), y])
        grad_w = T.grad(cost=cost, wrt=self.W)
        grad_b = T.grad(cost=cost, wrt=self.b)
        
        update = [
            [self.W, self.W - learning_rate * grad_w] ,
            [self.b, self.b - learning_rate * grad_b]
        ]
        index = T.lscalar()
        self.train_model = theano.function(
            inputs=[X, y, learning_rate],
            outputs=cost,
            updates=update
            #givens={
            #    X: x_data,#[index * batch_size: (index + 1) * batch_size],
            #    y: y_label#[index * batch_size: (index + 1) * batch_size]
            #}
        )
        self.predict_model = theano.function(
            inputs=[ X],
            outputs=pred_ids
            #givens={
            #    X: data#[index * batch_size: (index + 1) * batch_size],
            #}
        )
 
    
    #compile a theano function self.train that takes the matrix X, vector y, and learning rate as input
    def train(self, x_data, y_label, lr):

        #X = T.matrix('X')
        #y = T.ivector('y')
        
        #errors = T.mean(T.neq(self.y_pred, y))
        
        #data_size = x_data.shape[0]
        #if ((self.current_id+1) * batch_size >= data_size  ):
        #    self.current_id = 0
        train_cost = self.train_model( x_data, y_label, lr)
        #self.current_id += 1
        return train_cost

    #compile a theano function self.predict that takes a matrix X as input and returns a vector y of predictions
    def predict(self, data):
        #X = T.matrix('X')
        return self.predict_model(data)


    def fit(self, X, y, num_epochs=5, learning_rate=0.05, verbose=False):
        for i in range(num_epochs):
            cost = self.train(X, y, learning_rate)
            if verbose:
                print('cost at epoch {}: {}'.format(i, cost))

    
