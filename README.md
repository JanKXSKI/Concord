# Concord
Concord is an Unreal Engine plugin for sampling symbolic music from graphical probabilistic models in real time. Put another way, Concord lets you create and play intricate musical dice games. It aims to enable intuitive editing and efficient sampling in an interactive setting by leveraging the considerable capabilities and flexibility of the Unreal Engine.

## Core Functionality

At its core, Concord is about creating, editing and sampling from factor graphs over discrete random variables. The resulting distributions can be affected by dynamic parameters and observations. After running a sampler to find a likely configuration for the values of the discrete random variables, musical pattern data can be computed from these values and fed to a sound generator such as Unreal Engine's MetaSounds.

### Sampling from Factor Graphs

Concord relies on the conditional independence properties of factor graphs for efficient sampling. A parallelized implementation of the sum-product algorithm is applied in combination with ancestral sampling to find samples that exactly conform to the modelled distribution in acyclic factor graphs. Concord also provides a parallelized implementation of the max-sum algorithm to find the sample with the highest probability. Cycles in the factor graph prohibit the application of these algorithms. They can be automatically merged by collapsing factors, however the resulting factor graph's complexity can make exact sampling infeasible. For such cases, an implementation of Gibbs sampling is provided as well, which allows samples to be drawn from an approximate distribution.

### Crates

Before and between sampling runs, Concord models can be conditioned on observations and parameters to adapt the modelled distribution to the state of the interactive application. Parameters include both integers and floating point numbers that affect the scores computed by the factors. Observations are integers designating a state of the random variable they are applied to. Collections of parameters and observations are referred to as *crates* and can drastically change the behavior of Concord models. 

### Patterns

Symbolic music generated by a Concord model follows Tracker notation. Sampling runs produce *patterns* with a set number of *lines* corresponding to evenly spaced positions in a musical phrase. Each output fills one *column* of a *track* in the pattern with integers which differ in meaning depending on the type of column they are assigned to.

* A  **note** column contains MIDI note numbers at the positions of the notes' onsets. The value 0 is reserved and designates a no-op: if a note is currently playing, it will continue playing, otherwise the column will continue to be silent. Likewise, the value -1 is reserved and designates the end of a note in that position.

* An  **instrument** column contains the 1-based indices of the instrument that should be used to play the notes in the corresponding note column. The instrument column is optional since the name of a track in a Concord pattern equals the name of the instrument used to play notes on that track if the instrument column is missing or it's value is 0.

* A  **volume** column controls the volume of the corresponding note column with a value between 1 and 65. If the volume column is missing or its value is 0, the default volume of 65 is used.

* A  **delay** column contains offsets in ticks that delay the onset of notes in the corresponding positions. If the delay is larger or equal to the number of ticks per line, the corresponding note is not played at all.

## Model Graphs

Factor graphs defining a distribution over patterns can have a large number of random variables, parameters, factors and outputs. A Concord model is an abstraction of a factor graph that enables designers to reason about multidimensional arrays of these respective elements and allows for composition and instancing of models. An essential part of the Concord toolset is the graph editor used to specify Concord models. Nodes are connected in a directed, acyclic graph to build up the model in a similar way to other graph editors in Unreal Engine like Blueprint for visual scripting or MetaSounds for the creation of sound sources and effects.

### Names

Nodes of some types can be renamed by clicking on their title, by using the rename shortcut, or by using the input field under the *Node* category in the details tab while the node is selected. Uniqueness of names is enforced to avoid ambiguities, since these names are used to address nodes from outside the Concord model.

### Values

Nodes may have input and output *pins*. Such pins can be connected by the user to form the acyclic model graph. Connections within a Concord model represent the flow of multidimensional arrays of values of integer or floating point type. In the editor, integer pins are displayed in white, float pins in green. Each array has a *shape* associated with it, which contain the sizes of the array's dimensions from first to last. This will be familiar to users of numerical computation packages such as NumPy.

In Concord, shapes are static, meaning the dimensions of an array of values along a connection cannot change once the model is compiled down to a factor graph and used for sampling. Some input pins require their incoming connections to have a certain type or shape, however most nodes will adapt their behavior to the types and shapes of their inputs and thus can be used in many different contexts. A prevalent concept relating to this adaptive behavior is *broadcasting*. Many nodes with multiple input pins combine their inputs by repeating values in missing dimensions or dimensions of size 1. To that end, the dimension sizes of the input shapes are compared starting from the left. Dimension sizes must be equal, if they are not, all but one of the sizes must be 1 or missing. In that case, the shapes are broadcast by repeating values until all inputs supply the same amount of values. This repetition is not explicit and does not incur a performance cost.

### Compilation

Before a Concord model can be sampled from, it needs to be compiled to a factor graph. The compilation process includes finding the shapes and types of all node inputs and ouputs, collecting the individual factors and determining their neighbors in the factor graph (i.e. the random variables that affect the score computed by the factor), and accounting for composites and instances. The compiler will also merge cycles in the factor graph if a sampler that relies on exact inference is selected. This involves repeatedly collapsing factors until no more cycles are found, resulting in a larger neighborhood for the merged factors.

### Node Types

There are various types of nodes that make up a Concord model. Each node type has unique capabilities and responsibilities in a model and the resulting factor graph. What follows is a comprehensive list of node types as well as their functionalities, inputs and outputs. Many node types have additional properties in the details tab of the model editor when a node of such a type is selected. The following sections contain detailed information on these properties as well. 

### Boxes

Box nodes contain multidimensional arrays of discrete random variables with a given shape and state count. They are the only source of random behavior within a Concord model. By default, the random variables follow a uniform distribution over the chosen state count.

#### Properties

The *Default Shape* property is used to set the dimensions of the box when no connection to the *Shape* input pin is made. Dimensions can be added and removed, and their size and name can be set. Dimension names are simply available for display purposes, to help navigate a model with large boxes. The *State Count* property is an integer greater than 1. All random variables contained in the box can take on an integer value between 0 and N - 1 (inclusive), where N is the state count. By default, boxes expose output pins for individual random variables. If those are not needed, they can be toggled with the *Enable Individual Pins* property.

#### Input Pins

If the *Shape* input pin is connected to another pin, the shape of the box node is set to the shape of the connection at compile time. The incoming values are ignored. A box node's shape can thus depend on the shape of another node, e.g. a parameter whose shape can be set externally. If no connection to the *Shape* input pin is made, the box' default shape set in the details tab is used instead.

#### Output Pins
    
The *State Set* output pin is useful in cases where the shape of another box or parameter depends on the state count of a box. It outputs a 1-dimensional array with consecutive integer values from 0 to N - 1, where N is the state count. The main output pin of a box is the *All* pin, which simply outputs the states of the random variables within the box. Output pins for individual random variables are displayed below the *All* pin. When a variation has been sampled in the editor, the current state of the random variable and its associated entropy are shown next to the pin.

### Parameters

Parameter nodes feed multidimensional arrays of values of integral or floating point type into a Concord model. Values can have various sources. They can be given directly in the properties of the parameter, they can be loaded from a Crate asset, they can be set externally by game logic or they can act as an input when using the model containing the parameter as a composite or instance in another model.

#### Properties

The *Default Values* property can be used to specify the parameter values when no other parameter source is active. If the *Default Shape* property is empty, the parameter's default shape is simply a 1-dimensional array containing the default values, otherwise the default values are used to fill the default shape in a row-major fashion. If only a single default value is present, it is repeated to fill the default shape. As an alternative to the *Default Values* property, the parameter can also receive its default values from a crate asset that is available in the current Unreal project. When *Get Default Values From Crate* is enabled, the *Default Values Crate* property can be used to specify the desired crate. The *Default Values Crate Block Name* property functions as a key into the block map in the crate that matches the parameters numerical type. Parameters of floating point type have an additional *Trainable* flag in the properties.

#### Input Pins

The *Shape* input pin functions just as its equivalent on a box node and is especially useful on trainable parameters that correspond to probabilities of certain states of a box. In that case, the *State-Set* output pin on the box can be connected to the *Shape* input pin on the parameter node to obtain the desired shape.

### Transformers

Transformer nodes lazily perform indexing and arithmetic operations on their inputs and pass on the resulting multidimensional array. Concord comes with a large number of transformer nodes and it is designed to be easily extendable with custom transformers. Instead of manipulating values directly, transformers select and compose their input expressions to create output expressions which can then be evaluated in a given context at sampling time and be queried for the random variables they depend on. The default transformers include conversion functions between integer and floating point values, indexing and selection routines, musical computations like converting scale degrees to note numbers, mathematical operators, and so on. Transformers can depend on and manipulate the shape of the input arrays in a similar way to numerical computation packages like NumPy. The properties, input and output pins vary depending on the transformer's functionality. Most of them are fairly self-explanatory, so in the following only a selection of transformers is described in more detail.

#### Indexing Transformers

An indexing transformer outputs part of its input depending on its type, properties and additional input pin values. Such transformers can be static or dynamic, depending on whether the part of the input they pass on is known at compile time or depends on other runtime information respectively. Whenever possible, static indexing transformers should be chosen over dynamic ones, since the structure of the resulting factor graph will be sparser and thus more efficient when sampling.

Static indexing transformers include *Element*, which passes on a single value based on a multi-dimensional index set in its properties, *Get*, which removes one dimension from the input by choosing fixing it to a given index, and *Range*, which reduces the size of one dimension of the input by only passing on values between a first and last index with a given step in-between. The *Set* transformer is also a static indexing transformer. It passes on the values connected to its *Target* input pin, however the values at an index of a given dimension are replaced with the values connected to its *Source* input pin. The source values are broadcast to fit the required shape.

While the *Set (Dynamic)* transformer works much like its static counterpart, there are some subtleties when it comes to the dynamic variants of *Get*. Both *Get (Dynamic)* and *Get (Broadcasted)* have their target dimension index set in the properties and the dimension with said index will be removed when the values are passed on, however *Get (Dynamic)* outputs these values for each index in the incoming indices array while *Get (Broadcasted)* removes the chosen dimension from the values before broadcasting the indices to the resulting values array, and thus the output is always 1-dimensional.

#### Music Transformers

There are a number of transformers that perform musical computation. *Chord Root* checks the last dimension of its input for the root note when interpreting the values as MIDI note numbers constituting a chord, accounting for second inversions. *Position in Octave* wraps the distance between the input note and the root to the interval [0, 11]. There are also conversion routines between scale degrees and note numbers, namely *To Degree* and *To Note*. Modelling in scale degree space removes a large amount of superfluous dimensions when the music being modelled is known to stick to a certain scale. In Concord, scales are given by semitone offsets from the root. Thus, the major scale or Ionian mode is given by the 1-dimensional array of integers: 0, 2, 4, 5, 7, 9, 11. A scale degree of 1 corresponds to the root. Descending scale degrees below 1 simply descend the scale below the root, so a scale degree of 0 corresponds to a seventh one octave below the given root note. When converting from notes to scale degrees with the *To Degree* transformer, notes that do not fit into the given scale are converted to the *Default* input value.

#### Operator Transformers

There are a number of unary and binary operators that can be used to perform common arithmetic operations. Binary operators broadcast their inputs. If the *Outer* boolean property is enabled, a dimension can be selected in which all possible pairs will be combined with the chosen operator. Logical operators like *And* treat their inputs as true if they are non-zero and false otherwise. Boolean outputs are integers of value 0 or 1, corresponding to true and false respectively. The *Coalesce* operator passes on its first input if it is non-zero, otherwise it passes on the second input.  

#### Reduction Transformers

Reductions compute a single value from a collection of values. As such, they can impact the complexity of factor graphs. A factor depending on a reduction of the states of a large amount of random variables can slow down sampling considerably and even make exact inference infeasible. Nonetheless reductions are useful tools when used in moderation. The *Dimension Index* property is used to specify which dimension should be reduced to a single value with the chosen operation. By setting *Cumulative* to true, the chosen dimension will not be removed from the output, instead the value of the reduction at each value in the input will be passed on. *Start at Default* can be used to offset cumulative reductions so that the first value in the reduced dimension in the output is not equal to the first value of the input but rather the default value of the reduction used (e.g. 0 for sum, 1 for product).

#### Sequence Transformers

Sequence transformers treat the input along a dimension as a sequence of consecutive values. Which dimension to consider is given by the *Dimension Index* property. The *Flank* transformer outputs 0 if a value is equal to its predecessor in the input and 1 otherwise. The first value in the given dimension of the output can be chosen to be constant or depend on the last value for cyclic flank detection with the *Mode* property. *Flank Distance* outputs the number of values until a flank is reached in the given dimension of the input. The *Max Distance* property can be used to limit the horizon of the transformer, clamping the distance at a given value and thus enforcing more local structure. The *Interpolate* transformer takes an additional *Mask* input whose shape has to match the *Values* input in the given dimension. Values where the corresponding mask value is non-zero are passed on to the output, the remaining values are filled in based on the interpolation mode, which can be linear or constant. 

#### Shape Transformers

Shape transformers do not produce arithmetic expressions, but rather manipulate the shape of their inputs in various ways. The *Pad* and *Repeat* transformers implicitly add a dimension of size 1 to the back of their input shape so that individual values can be padded and repeated. *Reshape* can be used to achieve a particular broadcasting behavior by lining up the input shapes. When specifying the dimension sizes in the *Target Shape* property, a value of -1 can be used in one of the dimensions to automatically compute the size of said dimension based on the other sizes.

### Factors

Factor nodes treat their input as a multidimensional array of scores and thus define the probability distribution. The scores of each factor for a given configuration of random variable values are added up to find the logarithm of the unnormalized probability that the model takes on said configuration. Scores thus correspond to negative energies in a Boltzmann distribution. The higher the score, the more likely the current configuration. In order for factors to take effect, they need to be connected to one or more random variables via any number of transformers. A factor that depends on a large number of random variables with large state counts contributes significantly to the factor graph's complexity, which has an impact on sampling performance and might make exact inference infeasible altogether.

### Outputs

Output nodes can be queried for their values from outside the model and define the pattern that is computed from the random variables and the parameters. Any transformers involved in computing the output values do not affect sampling performance, since outputs are only evaluated when needed. When creating probabilistic models of symbolic music, it is useful to consider a reduced representation, like scale degrees instead of MIDI note numbers. The transformation from the reduced space to symbolic music that can be played back with a sound generator does not affect sampling performance if no factor relies on the output pattern data. Output nodes have a *Display Output* property that can be used to show the computed values when sampling a variation inside the editor, which can be a useful tool for debugging and sanity checks.
    
### Composites

Composite nodes allow for modularization of Concord models. They essentially copy the contents of the referenced model into the one that contains the composite node. Parameters and outputs of the referenced model correspond to inputs and outputs on the composite node respectively. Parameters that should not be exposed on a composite node can be set to *Local* in the referenced model. If no connection to an input pin on a composite node is made, the parameter in the referenced model will take on its default values and shape. If a connection is made, it is passed on into the referenced model and replaces the parameter default values. When connecting values that depend on random variables to a composite input, these random variables can be affected by the factors that the composite model contains. Output pins on composite nodes behave similarly to input pins. If an output in the composite model depends on a random variable within it, this random variable can be affected by factors in the model that contains the composite node.

### Instances

Instance nodes allow for chaining of Concord models. They sample the referenced model first and consequently treat the resulting values as observations when a sample from the model that contains the instance node is requested, thus reducing sampling complexity. The input and output pins on an instance node correspond to parameters and outputs in the referenced model in a similar way to composite nodes. However in contrast to composites, instances do not pass on connections into or out of the referenced model. Instead, the input values are computed before sampling from the instanced model, which fixes the output values of the instance node when sampling from the instancing model. It is illegal for an input of an instance node to depend on a random variable, since instance nodes sample and observe the instanced model before the instancing model is sampled from.

### Emissions

Emission nodes are used to setup and train Hidden Markov Models using Concord. An emission node is a shorthand for the factor nodes and trainable parameter nodes needed to setup a Hidden Markov Model. Its input pins only accept connections from *All* outputs on boxes, and both need to be connected. The first random variable in the hidden box is associated with a factor depending on trainable probabilities for each state the random variable can take, corresponding to the initial state distribution of the hidden Markov chain. Each pair of consecutive random variables in the hidden box is affected by the transition distribution and each pair of corresponding hidden and observed random variables is affected by the emission distribution. A hidden random variable can affect multiple observed random variables either by connecting the same hidden box to multiple emission nodes with different observed boxes or by choosing the shape of the observed box such that each hidden random variable is broadcast to multiple consecutive observed random variables. To allow this, the shape of the observed box may contain more dimensions than that of the hidden box, however the shapes must be identical up to and including the last dimension of the hidden box.

Emission nodes on their own have no effect on the model. They need to be trained on a data set by making use of Concord's learning capabilities. The parameter values resulting from training are the unnormalized log probabilities defining the initial, transition and emission distributions and can be set the same way conventional parameter values are set in Concord.

## Model Editor

The Concord model editor aims to facilitate the modelling process by providing live feedback, both edit-driven and executable. The shapes and types of node inputs and outputs are updated whenever the topology of the model graph changes. At any point the model can be compiled and run from the editor with the *Sample Variation* button in the toolbar or by hitting the space bar to observe a sample from the modelled distribution. This process is near instantaneous since compiling to a factor graph is cheap and sampling is fast for the family of models that can be considered for real-time applications. Designers can repeatedly generate samples from the model to inform their modelling decisions.

### Preview

Patterns sampled from a Concord model can be auditioned using the preview functionalities in the editor's details tab, allowing for evaluation of the model with the intended sound generator. The choice of instrumentation naturally influences the modelling process, just as composing for different instruments requires specific approaches. Therefore, playing back patterns from the modelled distribution with the chosen instrumentation is a vital form of feedback for the designer. In addition to the *Preview Sound*, there is also a *Preview Start Line* property, which can be used to offset the beginning of playback.

### Model Settings

The process of generating samples from a Concord model is affected by a number of settings in the details tab of the editor. Crucially, the *Default Sampler Factory* property offers a choice between exact sampling and approximate sampling using a Gibbs sampler. Additionally, default crates can be added to the model. The values in these crates will be used to observe random variables in boxes and set parameters whenever a sample from the model is requested.

#### Exact Sampler

Exact sampling is preferable for models of sufficiently low complexity. The marginal probability of an arbitrary random variable in the factor graph is found by means of the sum-product algorithm, then ancestral sampling is applied to find a truly fair sample under the factor graph's distribution. Models without cycles in the factor graph usually allow for exact sampling, unless very large boxes and state counts are used. Cycles can be merged automatically by collapsing factors to enable the use of the exact sampler even when cycles are present in the modelled factor graph. However, in cases where merging cycles leads to an increase of complexity above a given threshold, exact sampling becomes infeasible. The exact sampler can also be used to find the sample that maximizes the score by means of the max-sum algorithm. This sample is deterministically computed given the parameter values, allowing Concord to act as an optimizer on a discrete domain.

#### Gibbs Sampler

When exact sampling cannot be performed in reasonable time, Gibbs sampling can be used to obtain a sample that approximately follows the modelled distribution. The states of random variables are continuously updated by sampling from their conditional distribution, which can easily be evaluated given the modelled factor graph. As with all Markov Chain Monte Carlo approaches to sampling, a number of iterations of these updates need to be performed before the resulting samples approximately follow the factor graph's distribution. This number can be set using the *Burn In* property of the Gibbs sampler. In the editor, Gibbs sampling can also be used to find approximate marginal distributions of the random variables, which requires additional sampling iterations after the burn in is complete. The Gibbs samplers behavior can also be changed to look for the sample with maximal score by iteratively sampling from the conditional distributions.

### Feedback

In addition to the outputs resulting from a sampling run, the marginal and conditional distributions of the individual random variables can also be displayed in the editor. To that end, random variable pins in box nodes are equipped with indicators that change color based on the entropy of the random variable's marginal or conditional distribution. Thus the designer is provided with feedback on how sure the model is of its current configuration. The distributions can also be visualized in a bar chart using a viewer node. The designer can switch between marginal and conditional distributions at any time. Both have their merits when analyzing the model being built. The conditional distribution shows the probability of each state of a random variable given the current values of all other random variables, while the marginal distribution shows the probability of each state considering all possible configurations.

### Nativization

When sampling from a Concord model in the editor, the evaluation of factors and outputs relies on runtime polymorphism, obviating the need for a round trip to an IDE to compile code whenever changes to the model need to be analyzed. In order to reduce the overhead of virtual function calls and enable the training of parameters with machine learning algorithms, the Concord editor comes with tools to nativize the model being edited. Nativization will generate C++ code for factors and outputs of the modelled factor graph and use Unreal Engine's live coding features to compile and patch this code into the running Unreal Editor instance.

### Productivity

The editor provides a number of commands that are familiar to Unreal Engine users. In addition to classic editing tools like cut and paste, node organization is facilitate with alignment commands. There are also a number of actions to expedite common tasks when editing Concord models. The values from multiple output pins can be concatenated or stacked in a given dimension by creating a *Concat* or *Stack* transformer while the nodes containing the output pins are selected in the editor. The *From Parameter* and *To Output* meta actions can be used to promote the input and output pins of a selected composite or instance node to parameter and output nodes respectively. When dragging from an output pin the corresponding values can be separated by using the *Explode* meta action, which will create a *Get* transformer for each index in the first dimension and automatically create connections to any selected input pins. These meta actions are executed the same way as adding individual nodes to the graph.

## Learning

Concord is intended to be first and foremost a toolset for musical expression beyond the conventional deterministic and linear structure of a piece of music. Machine learning is a powerful part of this toolset. As in previous work on machine learning and music, the goal is not to train an artificial composer to replace a human artist, but rather to enable the creative use of data for musical expression. Concord models allow designers to reason about reduced representations of music in spaces with far fewer dimensions than the raw space of all possible notes in a phrase of a given length. By enforcing a carefully crafted structure, models can be trained to emit high quality samples with a high degree of confidence and very little data. Any factor graph whose complexity allows for exact inference with the sum-product algorithm can be trained with Concord's learning features, in particular Hidden Markov Models.

Data sets used for machine learning in Concord are collections of crates containing observations for random variables in a given model. The data set directory can be set in the details tab of the Concord editor, and may contain crates as Unreal assets in case a directory within the current Unreal project is chosen, or json files otherwise. Crate json files have the file extension *.ccdc* and follow the structure of the crate Unreal asset. They enable straight-forward compatibility with external data generation and pre-processing tools. The output of the training process is a crate asset containing the learned parameters. Concord supports running multiple learners with different random initializations concurrently, only the parameters which minimize the loss are retained.

### Baum-Welch Learner

The Baum-Welch algorithm can be used to estimate the parameters of a Hidden Markov Model from data. Concord models that contain at least one *Emission* node can make use of the Baum-Welch learner to find the initial, transition and emission distributions governing the hidden and observed random variables. The *Set Default Crates* property on the Baum-Welch learner can be enabled to observe values and set parameters from the Concord model's default crates before learning starts. This way, previously learned parameters can be further trained and training can be performed for specific non-trainable parameter configurations. There are additional settings for the standard deviation used when randomly initializing the parameters and when randomly offsetting parameters upon convergence.

## Symbols to Music

Concord's output can be used in limitless ways using Unreal's gameplay scripting system Blueprint. When treating the output as symbolic music, Unreal's DSP graph system MetaSounds can be used to turn the symbols into audio. Concord comes with a number of custom MetaSound nodes to facilitate this process. Additionally, an interface is provided to enable previewing symbolic music with MetaSounds right from the Concord editor.

There are two ways MetaSounds can be leveraged in combination with Concord: either by relying fully on a DSP graph for sound generation, or by loading a tracker module in a custom MetaSounds node to play back pattern data.

### MetaSounds DSP Graphs

When passing a Concord pattern to a MetaSound graph for audio generation, specific columns can be retrieved from the pattern inside the MetaSound by specifying a track name, column type and column index in the *Column Path* input of the *Concord Get Column* MetaSound node. Track name and column type are delimited by a slash, the column index is preceded by a colon. Type and index are optional and correspond to *Notes* and 0 respectively if missing. MetaSound's array and trigger functionalities can be used to drive sound generation based on the pattern data. A clock node is provided for convenient time keeping when playing back patterns this way. 

### Tracker Module Player

Music trackers have a rich history in computer games, they were even used in the original Unreal Tournament game. Tracker patterns are a great fit for probabilistic modelling of symbolic music as notes are arrange in a fixed grid. Concord supports play back of pattern data using module files in the Impulse Tracker format via the *Concord Player* MetaSound node. The instrument names used within the module file must correspond to the track names used in the pattern, though the instrument used for a track can be overridden by making use of a column of *Instrument* type containing 1-based instrument indices.

### Midi Import

In terms of digital symbolic music representations, MIDI is the predominant format. To enable the use of MIDI sequencers with Concord, MIDI files can be imported into the Unreal Editor as Concord patterns. Polyphony is achieved by assigning a unique column index to each concurrent voice in a MIDI track. Some note timings will be quantized by the import depending on the number of lines per beat and number of ticks per line specified. The imported pattern data can be used for playback as described above, or it can be converted to crate data and used as input for Concord models and machine learning.

### Pattern Server

It is also possible to use external sound generation software together with Concord by using the Unreal Editor or the packaged game as a pattern server. The Concord bridge component can receive crates and send patterns in json format, allowing for interoperability with other network-enabled tools.

## Actor Component

In Unreal, actor's are entities that can be placed in a level. Components are added to actors to enable certain functionalities. The Concord actor component handles interactions between the game world and a Concord model. Once a model has been assigned to the component, it exposes functions such as setting parameters, observing random variable values, generating a new sample from the model either synchronously or asynchronously and retrieving the outputs given the latest sample. These functions are accessible from Blueprint and can be used to easily script interactive symbolic music generation.

# Future Work
* Static polymorphism for box state count (like shape input)
* Contrastive Divergence learner for complex factor graphs
* A two stage learner that first makes inferences on high dof data before training on the reduced structure
* Additional ways to alter the pattern playing in a ConcordPlayer node within a Metasound from Blueprint (e.g. Stingers)
* Probabilistic program (box-like) node with sequential Monte Carlo sampler
* .it module playback via libxmp 4.2.6 is a little iffy, a more modern alternative would be nice...
* Reduce amount of code and size of index arrays generated when nativizing
* Node to observe random variable values directly in the Concord Model
* Ways to sample models via script, outside of Unreal editor

# NB

## Nativization
Everything is experimental, nativization especially so. The code generated when nativizing complex models may not compile due to huge array literals.

## Graph Synchronization
When changing the interface of a Concord model by adding/removing/renaming parameters or outputs, all other non-native models that use the edited model as a composite or instance are updated and saved. The edited graph is automatically saved as well.

## Concord Player: libxmp
* Stereo samples are not supported. An instrument whose name starts with "L_" or "R_" can get its pattern data from a track of the same name without a prefix, so you can emulate stereo samples by hard panning 2 instruments. Note that the stereo instrument pair needs to occupy neighboring instrument slots, with the left one coming first.
* Note-offs do not let the volume envelope run out.